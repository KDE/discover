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
#include <KConfig>
#include <KConfigGroup>
#include <KRun>
#include <KLocalizedString>
#include <KNotification>
#include <KPluginFactory>

DiscoverNotifier::DiscoverNotifier(QObject * parent)
    : QObject(parent)
    , m_verbose(false)
{
    configurationChanged();

    m_backends = BackendNotifierFactory().allBackends();
    foreach(BackendNotifierModule* module, m_backends) {
        connect(module, &BackendNotifierModule::foundUpdates, this, &DiscoverNotifier::updateStatusNotifier);
        connect(module, &BackendNotifierModule::needsRebootChanged, this, [this]() {
            if (!m_needsReboot) {
                m_needsReboot = true;
                showRebootNotification();
                Q_EMIT updatesChanged();
            }
        });
    }
    connect(&m_timer, &QTimer::timeout, this, &DiscoverNotifier::showUpdatesNotification);
    m_timer.setSingleShot(true);
    m_timer.setInterval(1000);
    updateStatusNotifier();
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
    if (state()==NoUpdates) {
        //it's not very helpful to notify that everyting is in order
        return;
    }

    auto e = KNotification::event(QStringLiteral("Update"), message(), extendedMessage(), iconName(), nullptr, KNotification::CloseOnTimeout, QStringLiteral("discoverabstractnotifier"));
    const QString name = i18n("Update");
    e->setDefaultAction(name);
    e->setActions({name});
    connect(e, QOverload<unsigned int>::of(&KNotification::activated), this, &DiscoverNotifier::showDiscover);
}

void DiscoverNotifier::updateStatusNotifier()
{
    uint securityCount = 0;
    for (BackendNotifierModule* module: m_backends)
        securityCount += module->securityUpdatesCount();

    uint count = securityCount;
    foreach(BackendNotifierModule* module, m_backends)
        count += module->updatesCount();

    if (m_count == count && m_securityCount == securityCount)
        return;

    if (state() != NoUpdates && m_count >= count) {
        m_timer.start();
    }

    m_securityCount = securityCount;
    m_count = count;
    emit updatesChanged();
}

DiscoverNotifier::State DiscoverNotifier::state() const
{
    if (m_needsReboot)
        return RebootRequired;
    else if (m_securityCount)
        return SecurityUpdates;
    else if (m_count)
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
    }
    return QString();
}

QString DiscoverNotifier::extendedMessage() const
{
    if (m_count > 0 && m_securityCount > 0) {
        QString allUpdates = i18ncp("First part of '%1, %2'",
                                    "1 package to update", "%1 packages to update", m_count);

        QString securityUpdates = i18ncp("Second part of '%1, %2'",
                                         "of which 1 is security update", "of which %1 are security updates", m_securityCount);

        return i18nc("%1 is '%1 packages to update' and %2 is 'of which %1 is security updates'",
                     "%1, %2", allUpdates, securityUpdates);
    } else if (m_count > 0) {
        return i18np("1 package to update", "%1 packages to update", m_count);
    } else if (m_securityCount > 0) {
        return i18np("1 security update", "%1 security updates", m_securityCount);
    } else {
        return i18n("No packages to update");
    }
}

void DiscoverNotifier::recheckSystemUpdateNeeded()
{
    foreach(BackendNotifierModule* module, m_backends)
        module->recheckSystemUpdateNeeded();
}

uint DiscoverNotifier::securityUpdatesCount() const
{
    return m_securityCount;
}

uint DiscoverNotifier::updatesCount() const
{
    return m_count;
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
    KNotification *notification = new KNotification(QLatin1String("notification"), KNotification::Persistent | KNotification::DefaultEvent);
    notification->setIconName(QStringLiteral("system-software-update"));
    notification->setActions(QStringList{QLatin1String("Restart")});
    notification->setTitle(i18n("Restart is required"));
    notification->setText(i18n("The system needs to be restarted for the updates to take effect."));

    connect(notification, &KNotification::action1Activated, this, [] () {
        QDBusInterface interface(QStringLiteral("org.kde.ksmserver"), QStringLiteral("/KSMServer"), QStringLiteral("org.kde.KSMServerInterface"), QDBusConnection::sessionBus());
        interface.asyncCall(QStringLiteral("logout"), 0, 1, 2); // Options: do not ask again | reboot | force
    });

    notification->sendEvent();
}
