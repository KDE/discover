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
#include <KConfig>
#include <KConfigGroup>
#include <KRun>
#include <KLocalizedString>
#include <KIconLoader>
#include <KNotification>
#include <KPluginFactory>
#include <QMenu>

DiscoverNotifier::DiscoverNotifier(QObject * parent)
    : QObject(parent)
    , m_verbose(false)
{
    configurationChanged();

    m_backends = BackendNotifierFactory().allBackends();
    for(BackendNotifierModule* module : m_backends) {
        connect(module, &BackendNotifierModule::foundUpdates, this, &DiscoverNotifier::updateStatusNotifier);
    }
    connect(&m_timer, &QTimer::timeout, this, &DiscoverNotifier::showUpdatesNotification);
    m_timer.setSingleShot(true);
    m_timer.setInterval(180000);
    updateStatusNotifier();
}

DiscoverNotifier::~DiscoverNotifier()
{
}

void DiscoverNotifier::configurationChanged()
{
    KConfig notifierConfig(QStringLiteral("muon-notifierrc"), KConfig::NoGlobals);

    KConfigGroup notifyTypeGroup(&notifierConfig, "NotificationType");
    m_verbose = notifyTypeGroup.readEntry("Verbose", false);
}

void DiscoverNotifier::showMuon()
{
    KRun::runCommand(QStringLiteral("muon-discover --mode update"), nullptr);
}

bool DiscoverNotifier::isSystemUpToDate() const
{
    for(BackendNotifierModule* module : m_backends) {
        if(!module->isSystemUpToDate())
            return false;
    }
    return true;
}

void DiscoverNotifier::showUpdatesNotification()
{
    //TODO: Better message strings
    QString msg = message();
    if (m_verbose) {
        msg += QLatin1Char(' ') + extendedMessage();
    }

    KNotification::event(QStringLiteral("Update"), i18n("System update available"), msg, QStringLiteral("system-software-update"), nullptr, KNotification::CloseOnTimeout, QStringLiteral("muonabstractnotifier"));
}

void DiscoverNotifier::updateStatusNotifier()
{
    if (!isSystemUpToDate()) {
        m_timer.start();
    }
    emit updatesChanged();
}

DiscoverNotifier::State DiscoverNotifier::state() const
{
    bool security = false, normal = false;

    for(BackendNotifierModule* module : m_backends) {
        security |= module->securityUpdatesCount()>0;
        normal |= security || module->updatesCount()>0;
        if (security)
            break;
    }
    if (security)
        return SecurityUpdates;
    else if (normal)
        return NormalUpdates;
    else
        return NoUpdates;
}

QString DiscoverNotifier::iconName() const
{
    switch(state()) {
        case SecurityUpdates:
            return QLatin1String("security-low");
        case NormalUpdates:
            return QLatin1String("security-high");
        case NoUpdates:
            return QLatin1String("security-high");
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
    }
    return QString();
}

QString DiscoverNotifier::extendedMessage() const
{
    uint securityCount = securityUpdatesCount();
    uint count = updatesCount() + securityCount;
    if (count > 0 && securityCount > 0) {
        QString allUpdates = i18ncp("First part of '%1, %2'",
                                    "1 package to update", "%1 packages to update", count);

        QString securityUpdates = i18ncp("Second part of '%1, %2'",
                                         "of which 1 is security update", "of which %1 are security updates", securityCount);

        return i18nc("%1 is '%1 packages to update' and %2 is 'of which %1 is security updates'",
                     "%1, %2", allUpdates, securityUpdates);
    } else if (count > 0) {
        return i18np("1 package to update", "%1 packages to update", count);
    } else if (securityCount > 0) {
        return i18np("1 security update", "%1 security updates", securityCount);
    } else {
        return i18n("No packages to update");
    }
}

void DiscoverNotifier::recheckSystemUpdateNeeded()
{
    for(BackendNotifierModule* module : m_backends)
        module->recheckSystemUpdateNeeded();
}

uint DiscoverNotifier::securityUpdatesCount() const
{
    uint ret = 0;
    for(BackendNotifierModule* module : m_backends)
        ret += module->securityUpdatesCount();
    return ret;
}

uint DiscoverNotifier::updatesCount() const
{
    uint ret = 0;
    for(BackendNotifierModule* module : m_backends)
        ret += module->updatesCount();
    return ret + securityUpdatesCount();
}

QStringList DiscoverNotifier::loadedModules() const
{
    QStringList ret;
    for(BackendNotifierModule* module : m_backends)
        ret += QString::fromLatin1(module->metaObject()->className());
    return ret;
}
