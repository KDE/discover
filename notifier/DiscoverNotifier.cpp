/***************************************************************************
 *   Copyright © 2014 Aleix Pol Gonzalez <aleixpol@blue-systems.com>       *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU General Public License as        *
 *   published by the Free Software Foundation; either version 2 of        *
 *   the License or (at your option) version 3 or any later version        *
 *   accepted by the membership of KDE e.V. (or its successor approved     *
 *   by the membership of KDE e.V.), which shall act as a proxy            *
 *   defined in Section 14 of version 3 of the license.                    *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>. *
 ***************************************************************************/

#include "DiscoverNotifier.h"
#include "BackendNotifierFactory.h"
#include <QDebug>
#include <QDBusInterface>
#include <QDBusPendingReply>
#include <QNetworkConfigurationManager>
#include <KConfig>
#include <KConfigGroup>
#include <KRun>
#include <KLocalizedString>
#include <KNotification>
#include <KPluginFactory>

#include "../libdiscover/utils.h"

DiscoverNotifier::DiscoverNotifier(QObject * parent)
    : QObject(parent)
    , m_manager(new QNetworkConfigurationManager(this))
{
    configurationChanged();

    m_backends = BackendNotifierFactory().allBackends();
    foreach(BackendNotifierModule* module, m_backends) {
        connect(module, &BackendNotifierModule::foundUpdates, this, &DiscoverNotifier::updateStatusNotifier);
        connect(module, &BackendNotifierModule::needsRebootChanged, this, [this]() {
            if (!m_needsReboot) {
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
    m_timer.setInterval(1000);
    updateStatusNotifier();

    connect(m_manager, &QNetworkConfigurationManager::onlineStateChanged, this, &DiscoverNotifier::stateChanged);

    //Only fetch updates after the system is comfortably booted
    QTimer::singleShot(10000, this, &DiscoverNotifier::recheckSystemUpdateNeeded);
}

DiscoverNotifier::~DiscoverNotifier() = default;

void DiscoverNotifier::configurationChanged()
{
    KConfig notifierConfig(QStringLiteral("plasma-discover-notifierrc"), KConfig::NoGlobals);

    KConfigGroup notifyTypeGroup(&notifierConfig, "NotificationType");
    m_verbose = notifyTypeGroup.readEntry("Verbose", false);
}

void DiscoverNotifier::showDiscover()
{
    KRun::runCommand(QStringLiteral("plasma-discover"), nullptr);
}

void DiscoverNotifier::showDiscoverUpdates()
{
    KRun::runCommand(QStringLiteral("plasma-discover --mode update"), nullptr);
}

void DiscoverNotifier::showUpdatesNotification()
{
    if (state() != NormalUpdates && state() != SecurityUpdates) {
        //it's not very helpful to notify that everything is in order
        return;
    }

    auto e = KNotification::event(QStringLiteral("Update"), message(), {}, iconName(), nullptr, KNotification::CloseOnTimeout, QStringLiteral("discoverabstractnotifier"));
    const QString name = i18n("Update");
    e->setDefaultAction(name);
    e->setActions({name});
    connect(e, QOverload<unsigned int>::of(&KNotification::activated), this, &DiscoverNotifier::showDiscoverUpdates);
}

void DiscoverNotifier::updateStatusNotifier()
{
    const bool hasSecurityUpdates = kContains(m_backends, [](BackendNotifierModule* module) { return module->hasSecurityUpdates(); });
    const bool hasUpdates = hasSecurityUpdates || kContains(m_backends, [](BackendNotifierModule* module) { return module->hasUpdates(); });

    if (m_hasUpdates == hasUpdates && m_hasSecurityUpdates == hasSecurityUpdates )
        return;

    m_hasSecurityUpdates = hasSecurityUpdates;
    m_hasUpdates = hasUpdates;

    if (state() != NoUpdates) {
        m_timer.start();
    }

    emit stateChanged();
}

DiscoverNotifier::State DiscoverNotifier::state() const
{
    if (m_needsReboot)
        return RebootRequired;
    else if (!m_manager->isOnline())
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
    switch(state()) {
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
    }
    return QString();
}

QString DiscoverNotifier::message() const
{
    switch(state()) {
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
    }
    return QString();
}

void DiscoverNotifier::recheckSystemUpdateNeeded()
{
    foreach(BackendNotifierModule* module, m_backends)
        module->recheckSystemUpdateNeeded();
}

QStringList DiscoverNotifier::loadedModules() const
{
    QStringList ret;
    for(BackendNotifierModule* module : m_backends)
        ret += QString::fromLatin1(module->metaObject()->className());
    return ret;
}

void DiscoverNotifier::showRebootNotification()
{
    KNotification *notification = new KNotification(QStringLiteral("notification"), KNotification::Persistent | KNotification::DefaultEvent);
    notification->setIconName(QStringLiteral("system-software-update"));
    notification->setActions(QStringList{QLatin1String("Restart")});
    notification->setTitle(i18n("Restart is required"));
    notification->setText(i18n("The system needs to be restarted for the updates to take effect."));

    connect(notification, &KNotification::action1Activated, this, &DiscoverNotifier::reboot);

    notification->sendEvent();
}

void DiscoverNotifier::reboot()
{
    QDBusInterface interface(QStringLiteral("org.kde.ksmserver"), QStringLiteral("/KSMServer"), QStringLiteral("org.kde.KSMServerInterface"), QDBusConnection::sessionBus());
    interface.asyncCall(QStringLiteral("logout"), 0, 1, 2); // Options: do not ask again | reboot | force
}

void DiscoverNotifier::foundUpgradeAction(UpgradeAction* action)
{
    KNotification *notification = new KNotification(QStringLiteral("distupgrade-notification"), KNotification::Persistent | KNotification::DefaultEvent);
    notification->setIconName(QStringLiteral("system-software-update"));
    notification->setActions(QStringList{QLatin1String("Upgrade")});
    notification->setTitle(i18n("Upgrade available"));
    notification->setText(i18n("New version: %1", action->description()));

    connect(notification, &KNotification::action1Activated, this, [action] () {
        action->trigger();
    });

    notification->sendEvent();
}
