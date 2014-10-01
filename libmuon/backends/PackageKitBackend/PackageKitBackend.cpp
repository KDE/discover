/***************************************************************************
 *   Copyright © 2012 Aleix Pol Gonzalez <aleixpol@blue-systems.com>       *
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

#include "PackageKitBackend.h"
#include "PackageKitResource.h"
#include "PackageKitUpdater.h"
#include "AppPackageKitResource.h"
#include "PKTransaction.h"
#include <resources/AbstractResource.h>
#include <resources/StandardBackendUpdater.h>
#include <Transaction/TransactionModel.h>
#include <QStringList>
#include <QDebug>
#include <QTimer>
#include <QTimerEvent>
#include <PackageKit/Transaction>
#include <PackageKit/Daemon>
#include <PackageKit/Details>

#include <KLocalizedString>

MUON_BACKEND_PLUGIN(PackageKitBackend)

PackageKitBackend::PackageKitBackend(QObject* parent)
    : AbstractResourcesBackend(parent)
    , m_updater(new PackageKitUpdater(this))
    , m_refresher(0)
    , m_isFetching(0)
{
    bool b = m_appdata.open();
    Q_ASSERT(b && "must be able to open the appstream database");
    reloadPackageList();

    QTimer* t = new QTimer(this);
    connect(t, &QTimer::timeout, this, &PackageKitBackend::refreshDatabase);
    t->setInterval(60 * 60 * 1000);
    t->setSingleShot(false);
    t->start();
}

PackageKitBackend::~PackageKitBackend()
{
}

bool PackageKitBackend::isFetching() const
{
    return m_isFetching;
}

void PackageKitBackend::acquireFetching(bool f)
{
    if (f)
        m_isFetching++;
    else
        m_isFetching--;

    if ((!f && m_isFetching==0) || (f && m_isFetching==1)) {
        emit fetchingChanged();
    }
}

void PackageKitBackend::reloadPackageList()
{
    m_updatingPackages = m_packages;
    
    if (m_refresher) {
        disconnect(m_refresher, SIGNAL(finished(PackageKit::Transaction::Exit,uint)), this, SLOT(reloadPackageList()));
    }

    PackageKit::Transaction * t = PackageKit::Daemon::global()->getPackages();
    connect(t, SIGNAL(finished(PackageKit::Transaction::Exit,uint)), this, SLOT(getPackagesFinished(PackageKit::Transaction::Exit)));
    connect(t, SIGNAL(package(PackageKit::Transaction::Info, QString, QString)), SLOT(addPackage(PackageKit::Transaction::Info, QString, QString)));
    connect(t, SIGNAL(errorCode(PackageKit::Transaction::Error,QString)), SLOT(transactionError(PackageKit::Transaction::Error,QString)));
    acquireFetching(true);

    PackageKit::Transaction * tUpdates = PackageKit::Daemon::global()->getUpdates();
    connect(tUpdates, SIGNAL(finished(PackageKit::Transaction::Exit,uint)), this, SLOT(getUpdatesFinished(PackageKit::Transaction::Exit,uint)));
    connect(tUpdates, SIGNAL(package(PackageKit::Transaction::Info, QString, QString)), SLOT(addPackageToUpdate(PackageKit::Transaction::Info,QString,QString)));
    connect(tUpdates, SIGNAL(errorCode(PackageKit::Transaction::Error,QString)), SLOT(transactionError(PackageKit::Transaction::Error,QString)));
    acquireFetching(true);
}

void PackageKitBackend::addPackage(PackageKit::Transaction::Info info, const QString &packageId, const QString &summary)
{
    QString packageName = PackageKit::Daemon::global()->packageName(packageId);
    if (AbstractResource* r = m_updatingPackages.value(packageName)) {
        qobject_cast<PackageKitResource*>(r)->addPackageId(info, packageId, summary);
    } else {
        PackageKitResource* newResource = 0;
        QList<Appstream::Component> components = m_appdata.findComponentsByPackageName(packageName);

        if (!components.isEmpty())
            newResource = new AppPackageKitResource(packageId, info, summary, components.first(), this);
        else
            newResource = new PackageKitResource(packageId, info, summary, this);
        m_updatingPackages[packageName] = newResource;
    }
}

void PackageKitBackend::getPackagesFinished(PackageKit::Transaction::Exit exit)
{
    Q_ASSERT(m_isFetching);

    if (exit != PackageKit::Transaction::ExitSuccess) {
        qWarning() << "error while fetching details" << exit;
    }

    m_packages = m_updatingPackages;
    QStringList ids;
    foreach(AbstractResource* res, m_updatingPackages) {
        ids += qobject_cast<PackageKitResource*>(res)->availablePackageId();
    }
    acquireFetching(false);

//  PackageKit has a maximum of packages to process called PK_TRANSACTION_MAX_PACKAGES_TO_PROCESS
//  which is 5200 today. To workaround that, we'll create different transactions that we'll process
//  one after the other.

    for(int i=0, step=1000; i<m_updatingPackages.count(); i+=step) {
        QStringList chunk = ids.mid(i, qMin(step, m_updatingPackages.count()-i));
        m_transactionQueue.append([chunk]() { return PackageKit::Daemon::global()->getDetails(chunk); });
    }
    iterateTransactionQueue();
}

void PackageKitBackend::transactionError(PackageKit::Transaction::Error, const QString& message)
{
    qWarning() << "Transaction error: " << message << sender();
}

void PackageKitBackend::queueTransactionFinished(PackageKit::Transaction::Exit exit, uint)
{
    if (exit != PackageKit::Transaction::ExitSuccess) {
        qWarning() << "error while fetching details" << exit;
    }
    iterateTransactionQueue();
    acquireFetching(false);
}

void PackageKitBackend::iterateTransactionQueue()
{
    if (m_transactionQueue.isEmpty())
        return;

    acquireFetching(true);
    PackageKit::Transaction* transaction = m_transactionQueue.takeFirst()();
    connect(transaction, SIGNAL(details(PackageKit::Details)), SLOT(packageDetails(PackageKit::Details)));
    connect(transaction, SIGNAL(errorCode(PackageKit::Transaction::Error,QString)), SLOT(transactionError(PackageKit::Transaction::Error,QString)));
    connect(transaction, SIGNAL(finished(PackageKit::Transaction::Exit,uint)), SLOT(queueTransactionFinished(PackageKit::Transaction::Exit,uint)));
}

void PackageKitBackend::packageDetails(const PackageKit::Details& details)
{
    PackageKitResource* res = qobject_cast<PackageKitResource*>(m_updatingPackages.value(PackageKit::Daemon::packageName(details.packageId())));
    Q_ASSERT(res);
    res->setDetails(details);
}

void PackageKitBackend::refreshDatabase()
{
    if (!m_refresher) {
        m_refresher = PackageKit::Daemon::global()->refreshCache(false);
        connect(m_refresher, SIGNAL(finished(PackageKit::Transaction::Exit,uint)), SLOT(reloadPackageList()));
    } else {
        qWarning() << "already resetting";
    }
}

QVector<AbstractResource*> PackageKitBackend::allResources() const
{
    return m_packages.values().toVector();
}

AbstractResource* PackageKitBackend::resourceByPackageName(const QString& name) const
{
    return m_packages[name];
}

QList<AbstractResource*> PackageKitBackend::searchPackageName(const QString& searchText)
{
    QList<AbstractResource*> ret;
    for(AbstractResource* res : m_packages.values()) {
        if (res->name().contains(searchText, Qt::CaseInsensitive)) {
//             kDebug() << "Got one" << res->name();
            ret += res;
        }
    }
    return ret;
}

int PackageKitBackend::updatesCount() const
{
    return m_updatesPackageId.count();
}

void PackageKitBackend::removeTransaction(Transaction* t)
{
    qDebug() << "Remove transaction:" << t->resource()->packageName() << "with" << m_transactions.size() << "transactions running";
    int count = m_transactions.removeAll(t);
    Q_ASSERT(count==1);
    Q_UNUSED(count)
    TransactionModel::global()->removeTransaction(t);
}

void PackageKitBackend::installApplication(AbstractResource* app, AddonList )
{
    installApplication(app);
}

void PackageKitBackend::installApplication(AbstractResource* app)
{
    PKTransaction* t = new PKTransaction(app, Transaction::InstallRole);
    m_transactions.append(t);
    TransactionModel::global()->addTransaction(t);
    t->start();
}

void PackageKitBackend::cancelTransaction(AbstractResource* app)
{
    for (Transaction* t : m_transactions) {
        PKTransaction* pkt = qobject_cast<PKTransaction*>(t);
        if (pkt->resource() == app) {
            if (pkt->transaction()->allowCancel()) {
                pkt->transaction()->cancel();
                int count = m_transactions.removeAll(t);
                Q_ASSERT(count==1);
                Q_UNUSED(count)
                //TransactionModel::global()->cancelTransaction(t);
            } else {
                qWarning() << "trying to cancel a non-cancellable transaction: " << app->name();
            }
            break;
        }
    }
}

void PackageKitBackend::removeApplication(AbstractResource* app)
{
    Q_ASSERT(!isFetching());
    PKTransaction* t = new PKTransaction(app, Transaction::RemoveRole);
    m_transactions.append(t);
    TransactionModel::global()->addTransaction(t);
    t->start();
}

QList<AbstractResource*> PackageKitBackend::upgradeablePackages() const
{
    QList<AbstractResource*> ret;
    for(const QString& pkgid : m_updatesPackageId) {
        ret += m_packages[PackageKit::Daemon::packageName(pkgid)];
    }
    return ret;
}

void PackageKitBackend::addPackageToUpdate(PackageKit::Transaction::Info info, const QString& packageId, const QString& summary)
{
    Q_UNUSED(summary);
    if (info != PackageKit::Transaction::InfoBlocked) {
        m_updatesPackageId += packageId;
    }
}

void PackageKitBackend::getUpdatesFinished(PackageKit::Transaction::Exit, uint)
{
    acquireFetching(false);
    emit updatesCountChanged();
}

bool PackageKitBackend::isPackageNameUpgradeable(const QString& name) const
{
    foreach (const QString& pkgid, m_updatesPackageId) {
        if (PackageKit::Daemon::packageName(pkgid) == name)
            return true;
    }
    return false;
}

AbstractBackendUpdater* PackageKitBackend::backendUpdater() const
{
    return m_updater;
}

//TODO
AbstractReviewsBackend* PackageKitBackend::reviewsBackend() const { return 0; }

#include "PackageKitBackend.moc"
