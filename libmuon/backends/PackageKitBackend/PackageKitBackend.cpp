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
#include "PKTransaction.h"
#include <resources/AbstractResource.h>
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
{
    populateCache();
    emit backendReady();
}

void PackageKitBackend::populateCache()
{
    emit reloadStarted();
    PackageKit::Transaction* t = new PackageKit::Transaction(this);
    connect(t, SIGNAL(finished(PackageKit::Transaction::Exit,uint)), this, SIGNAL(reloadFinished()));
    connect(t, SIGNAL(finished(PackageKit::Transaction::Exit,uint)), t, SLOT(deleteLater()));
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
    qDebug() << "done" << t->resource()->packageName() << m_transactions.size();
    int count = m_transactions.removeAll(t);
    Q_ASSERT(count==1);
    emit transactionRemoved(t);
}

void PackageKitBackend::installApplication(AbstractResource* app, const QHash<QString, bool>& )
{
    installApplication(app);
}

void PackageKitBackend::installApplication(AbstractResource* app)
{
    PackageKit::Transaction* installTransaction = new PackageKit::Transaction(this);
    installTransaction->installPackage(qobject_cast<PackageKitResource*>(app)->package());
    PKTransaction* t = new PKTransaction(app, InstallApp, installTransaction);
    m_transactions.append(t);
    emit transactionAdded(t);
    emit transactionsEvent(StartedCommitting, t);
}

void PackageKitBackend::cancelTransaction(AbstractResource* app)
{
    foreach(Transaction* t, m_transactions) {
        PKTransaction* pkt = qobject_cast<PKTransaction*>(t);
        if(pkt->resource() == app) {
            if(pkt->transaction()->allowCancel()) {
                pkt->transaction()->cancel();
                removeTransaction(t);
                emit transactionCancelled(t);
            } else
                kWarning() << "trying to cancel a non-cancellable transaction: " << app->name();
            break;
        }
    }
}

void PackageKitBackend::removeApplication(AbstractResource* app)
{
    PackageKit::Transaction* removeTransaction = new PackageKit::Transaction(this);
    removeTransaction->removePackage(qobject_cast<PackageKitResource*>(app)->package());
    PKTransaction* t = new PKTransaction(app, RemoveApp, removeTransaction);
    m_transactions.append(t);
    emit transactionAdded(t);
    emit transactionsEvent(FinishedCommitting, t);
    qDebug() << "remove" << app->packageName();
}

QPair<TransactionStateTransition, Transaction*> PackageKitBackend::currentTransactionState() const
{
    if(m_transactions.isEmpty())
        return qMakePair<TransactionStateTransition, Transaction*>(FinishedCommitting, nullptr);
    else
        return qMakePair<TransactionStateTransition, Transaction*>(StartedCommitting, m_transactions.first());
}

QList<Transaction*> PackageKitBackend::transactions() const
{
    return m_transactions;
}

//TODO
AbstractReviewsBackend* PackageKitBackend::reviewsBackend() const { return 0; }
AbstractBackendUpdater* PackageKitBackend::backendUpdater() const { return 0; }
