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

#include "PackageKitBackend.h"
#include "PackageKitResource.h"
#include "AppPackageKitResource.h"
#include "AppstreamUtils.h"
#include <resources/AbstractResource.h>
#include <QStringList>
#include <QDebug>
#include <PackageKit/packagekit-qt2/Transaction>

#include <KPluginFactory>
#include <KLocalizedString>
#include <KAboutData>

K_PLUGIN_FACTORY(MuonPackageKitBackendFactory, registerPlugin<PackageKitBackend>(); )
K_EXPORT_PLUGIN(MuonPackageKitBackendFactory(KAboutData("muon-pkbackend","muon-pkbackend",ki18n("PackageKit Backend"),"0.1",ki18n("Install PackageKit data in your system"), KAboutData::License_GPL)))

PackageKitBackend::PackageKitBackend(QObject* parent, const QVariantList&)
	: AbstractResourcesBackend(parent)
{
    populateCache();
    emit backendReady();
}

void PackageKitBackend::populateCache()
{
    emit reloadStarted();
    PackageKit::Transaction* t = new PackageKit::Transaction(this);
    connect(t, SIGNAL(finished(PackageKit::Transaction::Exit,uint)), this, SIGNAL(reloadFinished()));
    connect(t, SIGNAL(package(PackageKit::Package)), SLOT(addPackage(PackageKit::Package)));
    t->getPackages();

    m_appdata = AppstreamUtils::fetchAppData("/tmp/appdata.xml");
}

void PackageKitBackend::addPackage(const PackageKit::Package& p)
{
    PackageKitResource* newResource = 0;
    QHash<QString, ApplicationData>::const_iterator it = m_appdata.constFind(p.name());
    if(it!=m_appdata.constEnd())
        newResource = new AppPackageKitResource(p, *it, this);
    else
        newResource = new PackageKitResource(p, this);
    m_packages += newResource;
}

QVector<AbstractResource*> PackageKitBackend::allResources() const
{
    return m_packages;
}

AbstractResource* PackageKitBackend::resourceByPackageName(const QString& name) const
{
    AbstractResource* ret = 0;
    for(AbstractResource* res : m_packages) {
        if(res->name()==name) {
            ret = res;
            break;
        }
    }
    return ret;
}

QStringList PackageKitBackend::searchPackageName(const QString& searchText)
{
    QStringList ret;
    for(AbstractResource* res : m_packages) {
        if(res->name().contains(searchText))
            ret += res->packageName();
    }
    return ret;
}

int PackageKitBackend::updatesCount() const
{
    int ret = 0;
    for(AbstractResource* res : m_packages) {
        if(res->state() == AbstractResource::Upgradeable) {
            ret++;
        }
    }
    return ret;
}

//TODO
AbstractReviewsBackend* PackageKitBackend::reviewsBackend() const { return 0; }
AbstractBackendUpdater* PackageKitBackend::backendUpdater() const { return 0; }
QList< Transaction* > PackageKitBackend::transactions() const { return QList<Transaction*>(); }
void PackageKitBackend::cancelTransaction(AbstractResource* ) {}
QPair<TransactionStateTransition, Transaction*> PackageKitBackend::currentTransactionState() const
{ return qMakePair<TransactionStateTransition, Transaction*>(FinishedCommitting, nullptr); }
void PackageKitBackend::installApplication(AbstractResource* , const QHash< QString, bool >& ) { }
void PackageKitBackend::removeApplication(AbstractResource* ) { }

