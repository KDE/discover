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

#include <KPluginFactory>
#include <QTimer>
#include <PackageKit/Daemon>

K_PLUGIN_FACTORY(MuonPackageKitNotifierFactory,
                 registerPlugin<PackageKitNotifier>();
                )
K_EXPORT_PLUGIN(MuonPackageKitNotifierFactory("muon-packagekit-notifier"))

const int UPDATE_INTERVAL_MS = 1000 * 60 * 30;

PackageKitNotifier::PackageKitNotifier(QObject* parent, const QVariantList &)
  : AbstractKDEDModule("packagekit", "muondiscover", parent), m_update(NoUpdate), m_timer(new QTimer(this))
{
    m_timer->setInterval(UPDATE_INTERVAL_MS);
    connect (m_timer, SIGNAL(timeout()), SLOT(recheckSystemUpdateNeeded()));
    recheckSystemUpdateNeeded();
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
    m_timer->stop();
    m_update = NoUpdate;
    PackageKit::Transaction * trans = PackageKit::Daemon::getUpdates(PackageKit::Transaction::FilterArch | PackageKit::Transaction::FilterLast);
    connect(trans, SIGNAL(package(PackageKit::Transaction::Info,QString,QString)), SLOT(package(PackageKit::Transaction::Info,QString,QString)));
    connect(trans, SIGNAL(destroy()), trans, SLOT(deleteLater()));
    connect(trans, SIGNAL(finished(PackageKit::Transaction::Exit, uint)), SLOT(finished(PackageKit::Transaction::Exit, uint)));
}

void PackageKitNotifier::package(PackageKit::Transaction::Info info, const QString &/*packageID*/, const QString &/*summary*/)
{
    if (info == PackageKit::Transaction::InfoSecurity) {
        m_update = Security;
    } else if (m_update == NoUpdate) {
        m_update = Normal;
    }
}

void PackageKitNotifier::finished(PackageKit::Transaction::Exit /*exit*/, uint)
{
    if (m_update == Security) {
        setSystemUpToDate(false, AbstractKDEDModule::SecurityUpdate);
    } else if (m_update == Normal) {
        setSystemUpToDate(false, AbstractKDEDModule::NormalUpdate);
    } else if (m_update == NoUpdate) {
        setSystemUpToDate(true);
        m_timer->start();
    }
}

#include "PackageKitNotifier.moc"
