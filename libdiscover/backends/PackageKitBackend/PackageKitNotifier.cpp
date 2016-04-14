/***************************************************************************
 *   Copyright © 2013 Lukas Appelhans <l.appelhans@gmx.de>                 *
 *   Copyright © 2015 Aleix Pol Gonzalez <aleixpol@blue-systems.com>       *
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

#include "PackageKitNotifier.h"

#include <QTimer>
#include <PackageKit/Daemon>

PackageKitNotifier::PackageKitNotifier(QObject* parent)
    : BackendNotifierModule(parent)
    , m_update(NoUpdate)
    , m_securityUpdates(0)
    , m_normalUpdates(0)
{
if (PackageKit::Daemon::global()->isRunning()) {
        recheckSystemUpdateNeeded();
    }
    connect(PackageKit::Daemon::global(), &PackageKit::Daemon::networkStateChanged, this, &PackageKitNotifier::recheckSystemUpdateNeeded);
    connect(PackageKit::Daemon::global(), &PackageKit::Daemon::updatesChanged, this, &PackageKitNotifier::recheckSystemUpdateNeeded);
    connect(PackageKit::Daemon::global(), &PackageKit::Daemon::isRunningChanged, this, &PackageKitNotifier::recheckSystemUpdateNeeded);
}

PackageKitNotifier::~PackageKitNotifier()
{
}

void PackageKitNotifier::recheckSystemUpdateNeeded()
{
    if (PackageKit::Daemon::global()->isRunning()) {
        PackageKit::Transaction * trans = PackageKit::Daemon::getUpdates();
        trans->setProperty("normalUpdates", 0);
        trans->setProperty("securityUpdates", 0);
        trans->setProperty("update", NoUpdate);
        connect(trans, &PackageKit::Transaction::package, this, &PackageKitNotifier::package);
        connect(trans, &PackageKit::Transaction::finished, this, &PackageKitNotifier::finished);
    }
}

void PackageKitNotifier::package(PackageKit::Transaction::Info info, const QString &/*packageID*/, const QString &/*summary*/)
{
    PackageKit::Transaction * trans = qobject_cast<PackageKit::Transaction *>(sender());

    switch (info) {
        case PackageKit::Transaction::InfoBlocked:
            break; //skip, we ignore blocked updates
        case PackageKit::Transaction::InfoSecurity:
            trans->setProperty("update", qMax<int>(Security, trans->property("update").toInt()));
            trans->setProperty("securityUpdates", trans->property("securityUpdates").toInt()+1);
            break;
        default:
            trans->setProperty("update", qMax<int>(Normal, trans->property("update").toInt()));
            trans->setProperty("normalUpdates", trans->property("normalUpdates").toInt()+1);
            break;
    }
}

void PackageKitNotifier::finished(PackageKit::Transaction::Exit /*exit*/, uint)
{
    const PackageKit::Transaction * trans = qobject_cast<PackageKit::Transaction *>(sender());

    const uint normalUpdates = trans->property("normalUpdates").toInt();
    const uint securityUpdates = trans->property("securityUpdates").toInt();
    const Update update = Update(trans->property("update").toInt());

    const bool changed = update != m_update || normalUpdates != m_normalUpdates || securityUpdates != m_securityUpdates;

    m_normalUpdates = normalUpdates;
    m_securityUpdates = securityUpdates;
    m_update = update;

    if (changed) {
        Q_EMIT foundUpdates();
    }
}

bool PackageKitNotifier::isSystemUpToDate() const
{
    return m_update == NoUpdate;
}

uint PackageKitNotifier::securityUpdatesCount()
{
    return m_securityUpdates;
}

uint PackageKitNotifier::updatesCount()
{
    return m_normalUpdates;
}

