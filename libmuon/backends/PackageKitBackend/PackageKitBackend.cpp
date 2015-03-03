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
#include <QAction>

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

    QAction* updateAction = new QAction(this);
    updateAction->setIcon(QIcon::fromTheme("system-software-update"));
    updateAction->setText(i18nc("@action Checks the Internet for updates", "Check for Updates"));
    updateAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_R));
    connect(updateAction, &QAction::triggered, this, &PackageKitBackend::refreshDatabase);
    m_messageActions += updateAction;

    connect(PackageKit::Daemon::global(), &PackageKit::Daemon::updatesChanged, this, &PackageKitBackend::fetchUpdates);
    connect(PackageKit::Daemon::global(), &PackageKit::Daemon::isRunningChanged, this, &PackageKitBackend::checkDaemonRunning);
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
    Q_ASSERT(m_isFetching>=0);
}

void PackageKitBackend::reloadPackageList()
{
    m_updatingPackages = m_packages;
    m_updatingTranslationPackageToApp = m_translationPackageToApp;
    
    if (m_refresher) {
        disconnect(m_refresher, SIGNAL(finished(PackageKit::Transaction::Exit,uint)), this, SLOT(reloadPackageList()));
    }

    for(const Appstream::Component& component: m_appdata.allComponents()) {
        m_updatingPackages[component.id()] = new AppPackageKitResource(component, this);
        foreach (const QString& pkg, component.packageNames()) {
            m_updatingTranslationPackageToApp[pkg] += component.id();
        }
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
    const QString packageName = PackageKit::Daemon::packageName(packageId);
    QVector<AbstractResource*> r = resourcesByPackageName(packageName, true);
    if (r.isEmpty()) {
        r += new PackageKitResource(packageName, summary, this);
        m_updatingPackages[packageName] = r.last();
    }
    foreach(const auto & res, r)
        static_cast<PackageKitResource*>(res)->addPackageId(info, packageId, summary);
}

void PackageKitBackend::getPackagesFinished(PackageKit::Transaction::Exit exit)
{
    Q_ASSERT(m_isFetching);

    if (exit != PackageKit::Transaction::ExitSuccess) {
        qWarning() << "error while fetching details" << exit;
    }

    m_packages = m_updatingPackages;
    m_translationPackageToApp = m_updatingTranslationPackageToApp;
    acquireFetching(false);
}

void PackageKitBackend::transactionError(PackageKit::Transaction::Error, const QString& message)
{
    qWarning() << "Transaction error: " << message << sender();
}

void PackageKitBackend::packageDetails(const PackageKit::Details& details)
{
    QVector<AbstractResource*> resources = resourcesByPackageName(PackageKit::Daemon::packageName(details.packageId()), false);
    foreach(AbstractResource* res, resources)
        qobject_cast<PackageKitResource*>(res)->setDetails(details);
}

QVector<AbstractResource*> PackageKitBackend::resourcesByPackageName(const QString& name, bool updating) const
{
    const QHash<QString, QStringList> *dictionary = updating ? &m_updatingTranslationPackageToApp : &m_translationPackageToApp;
    const QHash<QString, AbstractResource*> *pkgs = updating ? &m_updatingPackages : &m_packages;

    QVector<AbstractResource*> ret;
    QStringList names = dictionary->value(name);
    foreach(const QString& name, names) {
        ret += pkgs->value(name);
    }
    return ret;
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
    QVector<AbstractResource*> ret;
    for(const QString& pkgid : m_updatesPackageId) {
        const QString pkgname = PackageKit::Daemon::packageName(pkgid);
        ret += resourcesByPackageName(pkgname, false);
        if (ret.isEmpty()) {
            qWarning() << "couldn't find resource for" << pkgid;
        }
    }
    return ret.toList();
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
    if (!m_updatesPackageId.isEmpty()) {
        acquireFetching(true);
        PackageKit::Transaction* transaction = PackageKit::Daemon::getDetails(m_updatesPackageId.toList());
        connect(transaction, SIGNAL(details(PackageKit::Details)), SLOT(packageDetails(PackageKit::Details)));
        connect(transaction, SIGNAL(errorCode(PackageKit::Transaction::Error,QString)), SLOT(transactionError(PackageKit::Transaction::Error,QString)));
        connect(transaction, SIGNAL(finished(PackageKit::Transaction::Exit,uint)), SLOT(getUpdatesDetailsFinished(PackageKit::Transaction::Exit,uint)));
    }

    acquireFetching(false);
    emit updatesCountChanged();
}

void PackageKitBackend::getUpdatesDetailsFinished(PackageKit::Transaction::Exit exit, uint)
{
    if (exit != PackageKit::Transaction::ExitSuccess) {
        qWarning() << "Couldn't figure out the updates on PackageKit backend" << exit;
    }
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

void PackageKitBackend::checkDaemonRunning()
{
    if (!PackageKit::Daemon::isRunning()) {
        qWarning() << "PackageKit stopped running!";
    }
}

AbstractBackendUpdater* PackageKitBackend::backendUpdater() const
{
    return m_updater;
}

QList<QAction*> PackageKitBackend::messageActions() const
{
    return m_messageActions;
}


//TODO
AbstractReviewsBackend* PackageKitBackend::reviewsBackend() const { return 0; }

#include "PackageKitBackend.moc"
