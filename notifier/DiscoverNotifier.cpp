/*
 *   SPDX-FileCopyrightText: 2014 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "DiscoverNotifier.h"
#include "BackendNotifierFactory.h"
#include "UnattendedUpdates.h"
#include <KLocalizedString>
#include <KNotificationJobUiDelegate>
#include <KPluginFactory>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusPendingCall>
#include <QDebug>
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
#include <QNetworkConfigurationManager>
#else
#include <QNetworkInformation>
#endif
#include <QProcess>

#include <KIO/ApplicationLauncherJob>
#include <KIO/CommandLauncherJob>

#include "../libdiscover/utils.h"
#include "updatessettings.h"
#include <chrono>

using namespace std::chrono_literals;

DiscoverNotifier::DiscoverNotifier(QObject *parent)
    : QObject(parent)
{
    m_settings = new UpdatesSettings(this);
    m_settingsWatcher = KConfigWatcher::create(m_settings->sharedConfig());
#if QT_VERSION >= QT_VERSION_CHECK(6, 3, 0)
    QNetworkInformation::instance()->load(QNetworkInformation::Feature::Reachability | QNetworkInformation::Feature::TransportMedium);
    connect(QNetworkInformation::instance(), &QNetworkInformation::reachabilityChanged, this, &DiscoverNotifier::stateChanged);
    connect(QNetworkInformation::instance(), &QNetworkInformation::transportMediumChanged, this, &DiscoverNotifier::stateChanged);
#endif

    refreshUnattended();
    connect(m_settingsWatcher.data(), &KConfigWatcher::configChanged, this, [this](const KConfigGroup &group, const QByteArrayList &names) {
        if (group.config()->name() == m_settings->config()->name() && group.name() == "Global" && names.contains("UseUnattendedUpdates")) {
            refreshUnattended();
        }
    });

    m_backends = BackendNotifierFactory().allBackends();
    for (BackendNotifierModule *module : std::as_const(m_backends)) {
        connect(module, &BackendNotifierModule::foundUpdates, this, &DiscoverNotifier::updateStatusNotifier);
        connect(module, &BackendNotifierModule::needsRebootChanged, this, [this]() {
            // If we are using offline updates, there is no need to badger the user to
            // reboot since it is safe to continue using the system in its current state
            if (!m_needsReboot && !m_settings->useUnattendedUpdates()) {
                m_needsReboot = true;
                showRebootNotification();
                Q_EMIT stateChanged();
                Q_EMIT needsRebootChanged(true);
            }
        });

        connect(module, &BackendNotifierModule::foundUpgradeAction, this, &DiscoverNotifier::foundUpgradeAction);
    }
    connect(&m_timer, &QTimer::timeout, this, &DiscoverNotifier::showUpdatesNotification);
    m_timer.setSingleShot(true);
    m_timer.setInterval(1s);
    updateStatusNotifier();

    // Only fetch updates after the system is comfortably booted
    QTimer::singleShot(0s, this, &DiscoverNotifier::recheckSystemUpdateNeeded);
}

DiscoverNotifier::~DiscoverNotifier() = default;

void DiscoverNotifier::showDiscover(const QString &xdgActivationToken)
{
    auto *job = new KIO::ApplicationLauncherJob(KService::serviceByDesktopName(QStringLiteral("org.kde.discover")));
    job->setStartupId(xdgActivationToken.toUtf8());
    job->setUiDelegate(new KNotificationJobUiDelegate(KJobUiDelegate::AutoErrorHandlingEnabled));
    job->start();

    if (m_updatesAvailableNotification) {
        m_updatesAvailableNotification->close();
    }
}

void DiscoverNotifier::showDiscoverUpdates(const QString &xdgActivationToken)
{
    auto *job = new KIO::CommandLauncherJob(QStringLiteral("plasma-discover"), {QStringLiteral("--mode"), QStringLiteral("update")});
    job->setUiDelegate(new KNotificationJobUiDelegate(KJobUiDelegate::AutoErrorHandlingEnabled));
    job->setDesktopName(QStringLiteral("org.kde.discover"));
    job->setStartupId(xdgActivationToken.toUtf8());
    job->start();

    if (m_updatesAvailableNotification) {
        m_updatesAvailableNotification->close();
    }
}

bool DiscoverNotifier::notifyAboutUpdates() const
{
    if (state() != NormalUpdates && state() != SecurityUpdates) {
        // it's not very helpful to notify that everything is in order
        return false;
    }

    if (m_settings->requiredNotificationInterval() < 0) {
        return false;
    }

    // To configure to a random value, execute:
    // kwriteconfig5 --file PlasmaDiscoverUpdates --group Global --key RequiredNotificationInterval 3600
    const QDateTime earliestNextNotificationTime = m_settings->lastNotificationTime().addSecs(m_settings->requiredNotificationInterval());
    if (earliestNextNotificationTime.isValid() && earliestNextNotificationTime > QDateTime::currentDateTimeUtc()) {
        return false;
    }

    m_settings->setLastNotificationTime(QDateTime::currentDateTimeUtc());
    m_settings->save();
    return true;
}

void DiscoverNotifier::showUpdatesNotification()
{
    if (m_updatesAvailableNotification) {
        m_updatesAvailableNotification->close();
    }

    if (!notifyAboutUpdates()) {
        return;
    }

    m_updatesAvailableNotification = KNotification::event(QStringLiteral("Update"),
                                                          message(),
                                                          {},
                                                          iconName(),
                                                          nullptr,
                                                          KNotification::CloseOnTimeout,
                                                          QStringLiteral("discoverabstractnotifier"));
    m_updatesAvailableNotification->setHint(QStringLiteral("resident"), true);
    const QString name = i18n("View Updates");
    m_updatesAvailableNotification->setDefaultAction(name);
    m_updatesAvailableNotification->setActions({name});
    connect(m_updatesAvailableNotification, QOverload<unsigned int>::of(&KNotification::activated), this, [this] {
        showDiscoverUpdates(m_updatesAvailableNotification->xdgActivationToken());
    });
}

void DiscoverNotifier::updateStatusNotifier()
{
    const bool hasSecurityUpdates = kContains(m_backends, [](BackendNotifierModule *module) {
        return module->hasSecurityUpdates();
    });
    const bool hasUpdates = hasSecurityUpdates || kContains(m_backends, [](BackendNotifierModule *module) {
                                return module->hasUpdates();
                            });

    if (m_hasUpdates == hasUpdates && m_hasSecurityUpdates == hasSecurityUpdates)
        return;

    m_hasSecurityUpdates = hasSecurityUpdates;
    m_hasUpdates = hasUpdates;

    if (state() != NoUpdates) {
        m_timer.start();
    }

    Q_EMIT stateChanged();
}

// we only want to do unattended updates when on an ethernet or wlan network
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
static bool isConnectionAdequate(const QNetworkConfiguration &network)
{
    return (network.bearerType() == QNetworkConfiguration::BearerEthernet || network.bearerType() == QNetworkConfiguration::BearerWLAN);
}
#elif QT_VERSION >= QT_VERSION_CHECK(6, 3, 0)
static bool isConnectionAdequate()
{
    const auto info = QNetworkInformation::instance();
    if (info->supports(QNetworkInformation::Feature::Metered)) {
        return !info->isMetered();
    } else {
        const auto transport = info->transportMedium();
        return transport == QNetworkInformation::TransportMedium::Ethernet || transport == QNetworkInformation::TransportMedium::WiFi;
    }
}
#endif

void DiscoverNotifier::refreshUnattended()
{
    m_settings->read();

    if (!notifyAboutUpdates()) {
        return;
    }

    const auto enabled = m_settings->useUnattendedUpdates()
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
        && m_manager->isOnline() && isConnectionAdequate(m_manager->defaultConfiguration());
#elif QT_VERSION >= QT_VERSION_CHECK(6, 3, 0)
        && QNetworkInformation::instance()->reachability() == QNetworkInformation::Reachability::Online && isConnectionAdequate();
#endif
    if (bool(m_unattended) == enabled)
        return;

    if (enabled) {
        m_unattended = new UnattendedUpdates(this);
    } else {
        delete m_unattended;
        m_unattended = nullptr;
    }
}

DiscoverNotifier::State DiscoverNotifier::state() const
{
    if (m_needsReboot)
        return RebootRequired;
    else if (m_isBusy)
        return Busy;
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    else if (m_manager && !m_manager->isOnline())
#else
    else if (QNetworkInformation::instance()->reachability() != QNetworkInformation::Reachability::Online)
#endif
        return Offline;
    else if (m_hasSecurityUpdates)
        return SecurityUpdates;
    else if (m_hasUpdates)
        return NormalUpdates;
    else
        return NoUpdates;
}

QString DiscoverNotifier::iconName() const
{
    switch (state()) {
    case SecurityUpdates:
        return QStringLiteral("update-high");
    case NormalUpdates:
        return QStringLiteral("update-low");
    case NoUpdates:
        return QStringLiteral("update-none");
    case RebootRequired:
        return QStringLiteral("system-reboot");
    case Offline:
        return QStringLiteral("offline");
    case Busy:
        return QStringLiteral("state-download");
    }
    return QString();
}

QString DiscoverNotifier::message() const
{
    switch (state()) {
    case SecurityUpdates:
        return i18n("Security updates available");
    case NormalUpdates:
        return i18n("Updates available");
    case NoUpdates:
        return i18n("System up to date");
    case RebootRequired:
        return i18n("Computer needs to restart");
    case Offline:
        return i18n("Offline");
    case Busy:
        return i18n("Applying unattended updates…");
    }
    return QString();
}

void DiscoverNotifier::recheckSystemUpdateNeeded()
{
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    if (!m_manager) {
        m_manager = new QNetworkConfigurationManager(this);
        connect(m_manager, &QNetworkConfigurationManager::onlineStateChanged, this, &DiscoverNotifier::stateChanged);
        if (!m_manager->isOnline()) {
            Q_EMIT stateChanged();
        }
    }
#endif

    for (BackendNotifierModule *module : std::as_const(m_backends))
        module->recheckSystemUpdateNeeded();

    refreshUnattended();
}

QStringList DiscoverNotifier::loadedModules() const
{
    QStringList ret;
    for (BackendNotifierModule *module : m_backends)
        ret += QString::fromLatin1(module->metaObject()->className());
    return ret;
}

void DiscoverNotifier::showRebootNotification()
{
    KNotification *notification = KNotification::event(QStringLiteral("UpdateRestart"),
                                                       i18n("Restart is required"),
                                                       i18n("The system needs to be restarted for the updates to take effect."),
                                                       QStringLiteral("system-software-update"),
                                                       nullptr,
                                                       KNotification::Persistent | KNotification::DefaultEvent,
                                                       QStringLiteral("discoverabstractnotifier"));

    notification->setActions(QStringList{i18nc("@action:button", "Restart")});
    notification->setDefaultAction(notification->actions().constFirst());
    connect(notification, &KNotification::action1Activated, this, &DiscoverNotifier::reboot);

    notification->sendEvent();
}

void DiscoverNotifier::reboot()
{
    auto method = QDBusMessage::createMethodCall(QStringLiteral("org.kde.LogoutPrompt"),
                                                 QStringLiteral("/LogoutPrompt"),
                                                 QStringLiteral("org.kde.LogoutPrompt"),
                                                 QStringLiteral("promptReboot"));
    QDBusConnection::sessionBus().asyncCall(method);
}

void DiscoverNotifier::foundUpgradeAction(UpgradeAction *action)
{
    KNotification *notification = new KNotification(QStringLiteral("distupgrade-notification"), KNotification::Persistent | KNotification::DefaultEvent);
    notification->setIconName(QStringLiteral("system-software-update"));
    notification->setActions(QStringList{i18nc("@action:button", "Upgrade")});
    notification->setTitle(i18n("Upgrade available"));
    notification->setText(i18n("New version: %1", action->description()));

    connect(notification, &KNotification::action1Activated, this, [action]() {
        action->trigger();
    });

    notification->sendEvent();
}

void DiscoverNotifier::setBusy(bool isBusy)
{
    if (isBusy == m_isBusy)
        return;

    m_isBusy = isBusy;
    Q_EMIT busyChanged(isBusy);
    Q_EMIT stateChanged();
}
