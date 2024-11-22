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
#include <QDBusConnectionInterface>
#include <QDBusMessage>
#include <QDBusPendingCall>
#include <QDebug>
#include <QNetworkInformation>
#include <QProcess>

#include <KIO/ApplicationLauncherJob>
#include <KIO/CommandLauncherJob>

#include "../libdiscover/UpdateModel/RefreshNotifierDBus.h"
#include "../libdiscover/utils.h"
#include "updatessettings.h"
#include <chrono>

using namespace std::chrono_literals;

namespace
{
bool isOnline()
{
    return !QNetworkInformation::instance() || QNetworkInformation::instance()->reachability() == QNetworkInformation::Reachability::Online;
}
} // namespace

DiscoverNotifier::DiscoverNotifier(QObject *parent)
    : QObject(parent)
    , m_settings(new UpdatesSettings(this))
{
    connect(&m_quitTimer, &QTimer::timeout, this, [] {
        qApp->quit();
    });

    refreshUnattended();

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
    QTimer::singleShot(0, this, &DiscoverNotifier::recheckSystemUpdateNeeded);

    // Listen to broadcasts from discover about notification changes.
    QDBusConnection::sessionBus().connect(QString(),
                                          RefreshNotifierDBus::path,
                                          RefreshNotifierDBus::interface,
                                          RefreshNotifierDBus::notifyNotifier,
                                          this,
                                          SLOT(recheckSystemUpdateNeeded()));
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

#warning it super awkward that the checker function sets as well
    m_settings->setLastNotificationTime(QDateTime::currentDateTimeUtc());
    m_settings->save();

#warning this makes no sense if discover is running wed still need to run unattended updates somehow
    if (QDBusConnection::sessionBus().interface()->isServiceRegistered(QStringLiteral("org.kde.discover"))) {
        return false;
    }
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

    m_updatesAvailableNotification =
        KNotification::event(QStringLiteral("Update"), message(), {}, iconName(), KNotification::CloseOnTimeout, QStringLiteral("discoverabstractnotifier"));
    m_updatesAvailableNotification->setHint(QStringLiteral("resident"), true);
    const QString name = i18n("View Updates");

    auto showUpdates = [this] {
        showDiscoverUpdates(m_updatesAvailableNotification->xdgActivationToken());
    };

    auto defaultAction = m_updatesAvailableNotification->addDefaultAction(name);
    connect(defaultAction, &KNotificationAction::activated, this, showUpdates);

    auto showUpdatesAction = m_updatesAvailableNotification->addAction(name);
    connect(showUpdatesAction, &KNotificationAction::activated, this, showUpdates);
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
static bool isConnectionAdequate()
{
    const auto info = QNetworkInformation::instance();
    if (!info) {
        return false; // no backend available (e.g. NetworkManager not running), assume not adequate
    }
    if (info->supports(QNetworkInformation::Feature::CaptivePortal) && info->isBehindCaptivePortal()) {
        return false;
    }
    if (info->supports(QNetworkInformation::Feature::Metered)) {
        return !info->isMetered();
    } else {
        const auto transport = info->transportMedium();
        return transport == QNetworkInformation::TransportMedium::Ethernet || transport == QNetworkInformation::TransportMedium::WiFi;
    }
}

void DiscoverNotifier::refreshUnattended()
{
    m_settings->read();

    if (!notifyAboutUpdates()) {
        return;
    }

    const auto enabled = m_settings->useUnattendedUpdates() && isOnline() && isConnectionAdequate();
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
    else if (!isOnline())
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
        return QStringLiteral("system-reboot-update");
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
    m_lastUpdate = QDateTime::currentDateTimeUtc();
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
                                                       KNotification::Persistent | KNotification::DefaultEvent,
                                                       QStringLiteral("discoverabstractnotifier"));

    auto restartAction = notification->addAction(i18nc("@action:button", "Update and Restart"));
    auto shutdownAction = notification->addAction(i18nc("@action:button", "Update and Shut Down"));
    connect(restartAction, &KNotificationAction::activated, this, &DiscoverNotifier::rebootPrompt);
    connect(shutdownAction, &KNotificationAction::activated, this, &DiscoverNotifier::shutdownPrompt);

    notification->sendEvent();
}

void DiscoverNotifier::rebootPrompt()
{
    auto method = QDBusMessage::createMethodCall(QStringLiteral("org.kde.LogoutPrompt"),
                                                 QStringLiteral("/LogoutPrompt"),
                                                 QStringLiteral("org.kde.LogoutPrompt"),
                                                 QStringLiteral("promptReboot"));
    QDBusConnection::sessionBus().asyncCall(method);
}

void DiscoverNotifier::shutdownPrompt()
{
    auto method = QDBusMessage::createMethodCall(QStringLiteral("org.kde.LogoutPrompt"),
                                                 QStringLiteral("/LogoutPrompt"),
                                                 QStringLiteral("org.kde.LogoutPrompt"),
                                                 QStringLiteral("promptShutDown"));
    QDBusConnection::sessionBus().asyncCall(method);
}

void DiscoverNotifier::promptAll()
{
    auto method = QDBusMessage::createMethodCall(QStringLiteral("org.kde.LogoutPrompt"),
                                                 QStringLiteral("/LogoutPrompt"),
                                                 QStringLiteral("org.kde.LogoutPrompt"),
                                                 QStringLiteral("promptAll"));
    QDBusConnection::sessionBus().asyncCall(method);
}

void DiscoverNotifier::foundUpgradeAction(UpgradeAction *action)
{
    updateStatusNotifier();

    if (!notifyAboutUpdates()) {
        return;
    }

    KNotification *notification = new KNotification(QStringLiteral("DistUpgrade"), KNotification::Persistent);
    notification->setIconName(QStringLiteral("system-software-update"));
    notification->setTitle(i18n("Upgrade available"));
    notification->setText(i18nc("A new distro release (name and version) is available for upgrade", "%1 is now available.", action->description()));
    notification->setComponentName(QStringLiteral("discoverabstractnotifier"));

    auto upgradeAction = notification->addAction(i18nc("@action:button", "Upgrade"));
    connect(upgradeAction, &KNotificationAction::activated, this, [action] {
        action->trigger();
    });

    connect(action, &UpgradeAction::showDiscoverUpdates, this, [this, notification]() {
        showDiscoverUpdates(notification->xdgActivationToken());
    });

    notification->sendEvent();
}

void DiscoverNotifier::setBusy(bool isBusy)
{
    if (isBusy == m_isBusy)
        return;

    m_isBusy = isBusy;
    Q_EMIT busyChanged();
    Q_EMIT stateChanged();

#warning state management needs some improvement. the notifier needs to up when it has a notification pending for reboot or a statusnotifier or is busy. in a way itd be tidier if reboot and statusnotifier was a separate dbus activated tool perhaps
    if (isBusy) {
        m_quitTimer.stop();
    } else {
        m_quitTimer.start(4s);
    }
}

void DiscoverNotifier::recheckSystemUpdateNeededAndNotifyApp()
{
    recheckSystemUpdateNeeded();
    RefreshNotifierDBus::notify(RefreshNotifierDBus::notifyApp);
}

#include "moc_DiscoverNotifier.cpp"
