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

#include "MuonNotifier.h"
#include "BackendNotifierFactory.h"
#include <KConfig>
#include <KConfigGroup>
#include <KRun>
#include <KLocalizedString>
#include <KIconLoader>
#include <KNotification>
#include <KPluginFactory>
#include <QMenu>

MuonNotifier::MuonNotifier(QObject * parent)
    : QObject(parent)
    , m_verbose(false)
{
    configurationChanged();

    m_backends = BackendNotifierFactory().allBackends();
    for(BackendNotifierModule* module : m_backends) {
        connect(module, &BackendNotifierModule::foundUpdates, this, &MuonNotifier::updateStatusNotifier);
    }
    updateStatusNotifier();
}

MuonNotifier::~MuonNotifier()
{
}

void MuonNotifier::configurationChanged()
{
    KConfig notifierConfig("muon-notifierrc", KConfig::NoGlobals);

    KConfigGroup notifyTypeGroup(&notifierConfig, "NotificationType");
    m_verbose = notifyTypeGroup.readEntry("Verbose", false);
}

void MuonNotifier::showMuon()
{
    KRun::runCommand("muon-updater", nullptr);
}

bool MuonNotifier::isSystemUpToDate() const
{
    for(BackendNotifierModule* module : m_backends) {
        if(!module->isSystemUpToDate())
            return false;
    }
    return true;
}

void MuonNotifier::updateStatusNotifier()
{
//     m_statusNotifier->setOverlayIconByName(iconName());
    if (!isSystemUpToDate()) {
        //TODO: Better message strings
        QString msg = message();
        if (m_verbose) {
            msg += ' ' + extendedMessage();
        }
//         m_statusNotifier->setToolTip(iconName(), msg, i18n("A system update is recommended"));
//         m_statusNotifier->setStatus(KStatusNotifierItem::Active);

        KNotification::event("Update", i18n("System update available"), msg, QStringLiteral("system-software-update"), nullptr, KNotification::CloseOnTimeout, "muonabstractnotifier");
    } else {
//         m_statusNotifier->setStatus(KStatusNotifierItem::Passive);
//         m_s:tatusNotifier->setToolTip(QString(), message(), i18n("Your system is up-to-date"));
    }
    emit updatesChanged();
}

MuonNotifier::State MuonNotifier::state() const
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

QString MuonNotifier::iconName() const
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

QString MuonNotifier::message() const
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

QString MuonNotifier::extendedMessage() const
{
    uint securityCount = securityUpdatesCount();
    uint count = updatesCount() + securityCount;
    if (count > 0 && securityCount > 0) {
        return i18n("%1 packages to update, of which %2 are security updates", count, securityCount);
    } else if (count > 0) {
        return i18n("%1 packages to update", count);
    } else if (securityCount > 0) {
        return i18n("%1 security updates", securityCount);
    } else {
        return i18n("No packages to update");
    }
}

void MuonNotifier::recheckSystemUpdateNeeded()
{
    for(BackendNotifierModule* module : m_backends)
        module->recheckSystemUpdateNeeded();
}

uint MuonNotifier::securityUpdatesCount() const
{
    uint ret = 0;
    for(BackendNotifierModule* module : m_backends)
        ret += module->securityUpdatesCount();
    return ret;
}

uint MuonNotifier::updatesCount() const
{
    uint ret = 0;
    for(BackendNotifierModule* module : m_backends)
        ret += module->updatesCount();
    return ret + securityUpdatesCount();
}

QStringList MuonNotifier::loadedModules() const
{
    QStringList ret;
    for(BackendNotifierModule* module : m_backends)
        ret += module->metaObject()->className();
    return ret;
}
