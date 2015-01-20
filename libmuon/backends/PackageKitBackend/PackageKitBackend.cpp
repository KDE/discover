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
    if (!b) {
        qWarning() << "Couldn't open the AppStream database";
    }
    reloadPackageList();

    QTimer* t = new QTimer(this);
    connect(t, &QTimer::timeout, this, &PackageKitBackend::refreshDatabase);
    t->setInterval(60 * 60 * 1000);
    t->setSingleShot(false);
    t->start();

    connect(PackageKit::Daemon::global(), &PackageKit::Daemon::updatesChanged, this, &PackageKitBackend::fetchUpdates);
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

    for(const Appstream::Component& component: m_appdata.allComponents()) {
        const QString name = component.packageNames().first();
        m_updatingPackages[name] = new AppPackageKitResource(component, this);
    }

    PackageKit::Transaction * t = PackageKit::Daemon::getPackages();
    connect(t, SIGNAL(finished(PackageKit::Transaction::Exit,uint)), this, SLOT(getPackagesFinished(PackageKit::Transaction::Exit)));
    connect(t, SIGNAL(package(PackageKit::Transaction::Info, QString, QString)), SLOT(addPackage(PackageKit::Transaction::Info, QString, QString)));
    connect(t, SIGNAL(errorCode(PackageKit::Transaction::Error,QString)), SLOT(transactionError(PackageKit::Transaction::Error,QString)));
    acquireFetching(true);

    fetchUpdates();
}

void PackageKitBackend::fetchUpdates()
{
    m_updatesPackageId.clear();

    PackageKit::Transaction * tUpdates = PackageKit::Daemon::getUpdates();
    connect(tUpdates, SIGNAL(finished(PackageKit::Transaction::Exit,uint)), this, SLOT(getUpdatesFinished(PackageKit::Transaction::Exit,uint)));
    connect(tUpdates, SIGNAL(package(PackageKit::Transaction::Info, QString, QString)), SLOT(addPackageToUpdate(PackageKit::Transaction::Info,QString,QString)));
    connect(tUpdates, SIGNAL(errorCode(PackageKit::Transaction::Error,QString)), SLOT(transactionError(PackageKit::Transaction::Error,QString)));
    acquireFetching(true);
}


void PackageKitBackend::addPackage(PackageKit::Transaction::Info info, const QString &packageId, const QString &summary)
{
    QString packageName = PackageKit::Daemon::packageName(packageId);
    PackageKitResource* r = qobject_cast<PackageKitResource*>(m_updatingPackages.value(packageName));
    if (!r) {
        r = new PackageKitResource(packageName, summary, this);
        m_updatingPackages[packageName] = r;
    }
    r->addPackageId(info, packageId, summary);
}

void PackageKitBackend::getPackagesFinished(PackageKit::Transaction::Exit exit)
{
    Q_ASSERT(m_isFetching);

    if (exit != PackageKit::Transaction::ExitSuccess) {
        qWarning() << "error while fetching details" << exit;
    }

    m_packages = m_updatingPackages;
    acquireFetching(false);
}

void PackageKitBackend::transactionError(PackageKit::Transaction::Error, const QString& message)
{
    qWarning() << "Transaction error: " << message << sender();
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
        m_refresher = PackageKit::Daemon::refreshCache(false);
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
        if (!ret.last()) {
            qWarning() << "couldn't find resource for" << pkgid;
        }
    }
    return ret;
}

void PackageKitBackend::addPackageToUpdate(PackageKit::Transaction::Info info, const QString& packageId, const QString& summary)
{
    if (info != PackageKit::Transaction::InfoBlocked) {
        m_updatesPackageId += packageId;
        addPackage(info, packageId, summary);
    }
}

void PackageKitBackend::getUpdatesFinished(PackageKit::Transaction::Exit, uint)
{
    PackageKit::Transaction* transaction = PackageKit::Daemon::getDetails(m_updatesPackageId.toList());
    connect(transaction, SIGNAL(details(PackageKit::Details)), SLOT(packageDetails(PackageKit::Details)));
    connect(transaction, SIGNAL(errorCode(PackageKit::Transaction::Error,QString)), SLOT(transactionError(PackageKit::Transaction::Error,QString)));
    connect(transaction, SIGNAL(finished(PackageKit::Transaction::Exit,uint)), SLOT(getUpdatesDetailsFinished(PackageKit::Transaction::Exit,uint)));

    emit updatesCountChanged();
}

void PackageKitBackend::getUpdatesDetailsFinished(PackageKit::Transaction::Exit, uint)
{
    acquireFetching(false);
}

bool PackageKitBackend::isPackageNameUpgradeable(PackageKitResource* res) const
{
    return !upgradeablePackageId(res).isEmpty();
}

QString PackageKitBackend::upgradeablePackageId(PackageKitResource* res) const
{
    QString name = res->packageName();
    for (const QString& pkgid: m_updatesPackageId) {
        if (PackageKit::Daemon::packageName(pkgid) == name)
            return pkgid;
    }
    return QString();
}

AbstractBackendUpdater* PackageKitBackend::backendUpdater() const
{
    return m_updater;
}

QList<QAction*> PackageKitBackend::messageActions() const
{
    return QList<QAction*>();
}


//TODO
AbstractReviewsBackend* PackageKitBackend::reviewsBackend() const { return 0; }

#include "PackageKitBackend.moc"
