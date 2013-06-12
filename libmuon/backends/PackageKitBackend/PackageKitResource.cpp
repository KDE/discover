/***************************************************************************
 *   Copyright © 2012 Aleix Pol Gonzalez <aleixpol@blue-systems.com>       *
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

#include "PackageKitResource.h"
#include <MuonDataSources.h>
#include <KGlobal>
#include <KLocale>
#include <PackageKit/packagekit-qt2/Daemon>

PackageKitResource::PackageKitResource(const QString &packageId, PackageKit::Transaction::Info info, const QString &summary, AbstractResourcesBackend* parent)
    : AbstractResource(parent)
    , m_packageId(packageId)
    , m_info(info)
    , m_summary(summary)
{
    setObjectName(m_packageId);
}

QString PackageKitResource::name()
{
    //return PackageKit::Daemon::global()->packageName(m_packageId);
    return m_packageId;
}

QString PackageKitResource::packageName() const
{
    return m_packageId;
    //return PackageKit::Daemon::global()->packageName(m_packageId);
}

QString PackageKitResource::comment()
{
    return QString();
    //return m_package.description();
}

QString PackageKitResource::longDescription() const
{
    return QString();
    //return m_package.changelog();
}

QUrl PackageKitResource::homepage() const
{
    return QString();
    //return QUrl(m_package.url());
}

QString PackageKitResource::icon() const
{
    return "muon-discover";
    //return PackageKit::Daemon::global()->packageIcon(m_packageId);
    //return m_package.iconPath();
}

QString PackageKitResource::license()
{
    return QString();
    //fetchDetails();
    //return m_package.license();
}

QList<PackageState> PackageKitResource::addonsInformation()
{
    return QList<PackageState>();
}

QString PackageKitResource::availableVersion() const
{
    return "0.1";
    //return PackageKit::Daemon::global()->packageVersion(m_packageId);
}

QString PackageKitResource::installedVersion() const
{
    return "0.1";
    //return PackageKit::Daemon::global()->packageVersion(m_packageId);
}

int PackageKitResource::downloadSize()
{
    return 0;
    //return m_package.size();
}

QString PackageKitResource::origin() const
{
    //FIXME
    return "PackageKit";
}

QString PackageKitResource::section()
{
    return "PK";
    //FIXME
    //return QString::number(m_package.group());
}

QUrl PackageKitResource::screenshotUrl()
{
    return KUrl(MuonDataSources::screenshotsSource(), "screenshot/"+packageName());
}

QUrl PackageKitResource::thumbnailUrl()
{
    return KUrl(MuonDataSources::screenshotsSource(), "thumbnail/"+packageName());
}

AbstractResource::State PackageKitResource::state()
{
    /*if(m_package.hasUpdateDetails())
        return Upgradeable;
    else {
        PackageKit::Package::Info info = m_package.info();
        if(info & PackageKit::Package::InfoInstalled) {
            return Installed;
        } else if(info & PackageKit::Package::InfoAvailable) {
            return None;
        }
    }*/
    return Installed;//Broken;
}

/*void PackageKitResource::updatePackage(const PackageKit::Package& p)
{
    if(p.info()==PackageKit::Package::UnknownInfo)
        kWarning() << "Received unknown Package::info() for " << p.name();
    bool changeState = p.info()!=m_package.info();
    m_package = p;
    if(changeState) {
        emit stateChanged();
    }
}*/

QStringList PackageKitResource::categories()
{
    return QStringList() << "System";
}

bool PackageKitResource::isTechnical() const
{
    return false;
}

void PackageKitResource::fetchDetails()
{
    /*if(m_package.hasDetails())
        return;

    PackageKit::Transaction* transaction = new PackageKit::Transaction(this);
    transaction->getDetails(m_package);
    connect(transaction, SIGNAL(package(PackageKit::Package)), SLOT(updatePackage(PackageKit::Package)));
    connect(transaction, SIGNAL(finished(PackageKit::Transaction::Exit,uint)), SIGNAL(licenseChanged()));
    connect(transaction, SIGNAL(finished(PackageKit::Transaction::Exit,uint)), transaction, SLOT(deleteLater()));*/
}

/*PackageKit::Package PackageKitResource::package() const
{
    return m_package;
}*/

void PackageKitResource::fetchChangelog()
{
    //TODO: implement
    emit changelogFetched(QString());
}
