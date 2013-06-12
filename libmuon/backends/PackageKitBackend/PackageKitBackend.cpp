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
//#include "AppPackageKitResource.h"
//#include "AppstreamUtils.h"
//#include "PKTransaction.h"
#include <resources/AbstractResource.h>
#include <resources/StandardBackendUpdater.h>
#include <Transaction/TransactionModel.h>
#include <QStringList>
#include <QDebug>
#include <PackageKit/packagekit-qt2/Transaction>

#include <KPluginFactory>
#include <KLocalizedString>
#include <KAboutData>
#include <KDebug>

K_PLUGIN_FACTORY(MuonPackageKitBackendFactory, registerPlugin<PackageKitBackend>(); )
K_EXPORT_PLUGIN(MuonPackageKitBackendFactory(KAboutData("muon-pkbackend","muon-pkbackend",ki18n("PackageKit Backend"),"0.1",ki18n("Install PackageKit data in your system"), KAboutData::License_GPL)))

PackageKitBackend::PackageKitBackend(QObject* parent, const QVariantList&)
    : AbstractResourcesBackend(parent)
    , m_updater(new StandardBackendUpdater(this))
{
    populateCache();
}

void PackageKitBackend::populateCache()
{
    //emit reloadStarted();
    m_packages += new PackageKitResource("foo", PackageKit::Transaction::InfoInstalled, "summ", this);
    PackageKit::Transaction* t = new PackageKit::Transaction(this);
    connect(t, SIGNAL(finished(PackageKit::Transaction::Exit,uint)), this, SLOT(finished(PackageKit::Transaction::Exit,uint)));
    connect(t, SIGNAL(destroy()), t, SLOT(deleteLater()));
    connect(t, SIGNAL(package(PackageKit::Transaction::Info, QString, QString)), SLOT(addPackage(PackageKit::Transaction::Info, QString, QString)));
    kDebug() << "Get all packages";
    t->getPackages();

    //m_appdata = AppstreamUtils::fetchAppData("/tmp/appdata.xml");
}

void PackageKitBackend::finished(PackageKit::Transaction::Exit,uint)
{
    kDebug() << "FINISHED";
    //emit reloadFinished();
    emit backendReady();
}

void PackageKitBackend::addPackage(PackageKit::Transaction::Info info, const QString &packageId, const QString &summary)
{
    PackageKitResource* newResource = 0;
    kDebug() << "Add new package" << packageId;
    //QHash<QString, ApplicationData>::const_iterator it = m_appdata.constFind(p.name());
    //if(it!=m_appdata.constEnd())
    //    newResource = new AppPackageKitResource(p, *it, this);
    //else
        newResource = new PackageKitResource(packageId, info, summary, this);
    m_packages += newResource;
}

QVector<AbstractResource*> PackageKitBackend::allResources() const
{
    kDebug() << "All resources" << m_packages.count();
    qDebug() << "All resources" << m_packages.count();
    return m_packages;
}

AbstractResource* PackageKitBackend::resourceByPackageName(const QString& name) const
{
    AbstractResource* ret = 0;
    for(AbstractResource* res : m_packages) {
        if(res->name()==name || res->packageName()==name) {
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
        if(res->name().contains(searchText, Qt::CaseInsensitive))
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

void PackageKitBackend::removeTransaction(Transaction* t)
{
    /*qDebug() << "done" << t->resource()->packageName() << m_transactions.size();
    int count = m_transactions.removeAll(t);
    Q_ASSERT(count==1);
    TransactionModel::global()->removeTransaction(t);*/
}

void PackageKitBackend::installApplication(AbstractResource* app, AddonList )
{
    //installApplication(app);
}

void PackageKitBackend::installApplication(AbstractResource* app)
{
    /*PackageKit::Transaction* installTransaction = new PackageKit::Transaction(this);
    installTransaction->installPackage(qobject_cast<PackageKitResource*>(app)->package());
    PKTransaction* t = new PKTransaction(app, Transaction::InstallRole, installTransaction);
    m_transactions.append(t);
    TransactionModel::global()->addTransaction(t);*/
}

void PackageKitBackend::cancelTransaction(AbstractResource* app)
{
    /*foreach(Transaction* t, m_transactions) {
        PKTransaction* pkt = qobject_cast<PKTransaction*>(t);
        if(pkt->resource() == app) {
            if(pkt->transaction()->allowCancel()) {
                pkt->transaction()->cancel();
                removeTransaction(t);
                TransactionModel::global()->cancelTransaction(t);
            } else
                kWarning() << "trying to cancel a non-cancellable transaction: " << app->name();
            break;
        }
    }*/
}

void PackageKitBackend::removeApplication(AbstractResource* app)
{
    /*PackageKit::Transaction* removeTransaction = new PackageKit::Transaction(this);
    removeTransaction->removePackage(qobject_cast<PackageKitResource*>(app)->package());
    PKTransaction* t = new PKTransaction(app, Transaction::RemoveRole, removeTransaction);
    m_transactions.append(t);
    TransactionModel::global()->addTransaction(t);
    qDebug() << "remove" << app->packageName();*/
}

QList<AbstractResource*> PackageKitBackend::upgradeablePackages() const
{
    QList<AbstractResource*> ret;
    for(AbstractResource* res : m_packages) {
        if(res->state() == AbstractResource::Upgradeable) {
            ret+=res;
        }
    }
    return ret;
}

AbstractBackendUpdater* PackageKitBackend::backendUpdater() const
{
    return m_updater;
}

//TODO
AbstractReviewsBackend* PackageKitBackend::reviewsBackend() const { return 0; }
