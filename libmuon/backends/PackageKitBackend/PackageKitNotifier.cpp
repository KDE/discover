/***************************************************************************
 *   Copyright © 2013 Lukas Appelhans <l.appelhans@gmx.de>                 *
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
    recheckSystemUpdateNeeded();
    connect(PackageKit::Daemon::global(), &PackageKit::Daemon::updatesChanged, this, &PackageKitNotifier::recheckSystemUpdateNeeded);
}

PackageKitNotifier::~PackageKitNotifier()
{
}

void PackageKitNotifier::configurationChanged()
{
    recheckSystemUpdateNeeded();
}

void PackageKitNotifier::recheckSystemUpdateNeeded()
{
    m_normalUpdates = 0;
    m_securityUpdates = 0;
    m_update = NoUpdate;
    PackageKit::Transaction * trans = PackageKit::Daemon::getUpdates();
    connect(trans, SIGNAL(package(PackageKit::Transaction::Info,QString,QString)), SLOT(package(PackageKit::Transaction::Info,QString,QString)));
    connect(trans, SIGNAL(finished(PackageKit::Transaction::Exit, uint)), SLOT(finished(PackageKit::Transaction::Exit, uint)));
}

void PackageKitNotifier::package(PackageKit::Transaction::Info info, const QString &/*packageID*/, const QString &/*summary*/)
{
    if (info == PackageKit::Transaction::InfoBlocked) {
        ;
    } else if (info == PackageKit::Transaction::InfoSecurity) {
        m_update = Security;
        m_securityUpdates++;
    } else if (m_update == NoUpdate) {
        m_update = Normal;
        m_normalUpdates++;
    }
}

void PackageKitNotifier::finished(PackageKit::Transaction::Exit /*exit*/, uint)
{
    Q_EMIT foundUpdates();
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

#include "PackageKitNotifier.moc"
