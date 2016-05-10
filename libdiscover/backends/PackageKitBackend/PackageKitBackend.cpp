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
#include "PackageKitSourcesBackend.h"
#include "PackageKitResource.h"
#include "PackageKitUpdater.h"
#include "AppPackageKitResource.h"
#include "PKTransaction.h"
#include "AppstreamReviews.h"
#include <resources/AbstractResource.h>
#include <resources/StandardBackendUpdater.h>
#include <resources/SourcesModel.h>
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
    , m_refresher(nullptr)
    , m_isFetching(0)
    , m_reviews(new AppstreamReviews(this))
{
    bool b = m_appdata.open();
    Q_ASSERT(b && "must be able to open the appstream database");
    if (!b) {
        qWarning() << "Couldn't open the AppStream database";

        auto msg = new QAction(i18n("Got it"), this);
        msg->setWhatsThis(i18n("Please make sure that Appstream is properly set up on your system"));
        msg->setPriority(QAction::HighPriority);
        connect(msg, &QAction::triggered, msg, [msg](){ msg->setVisible(false); });
        m_messageActions << msg;
    }
    reloadPackageList();

    QTimer* t = new QTimer(this);
    connect(t, &QTimer::timeout, this, &PackageKitBackend::refreshDatabase);
    t->setInterval(60 * 60 * 1000);
    t->setSingleShot(false);
    t->start();

    QAction* updateAction = new QAction(this);
    updateAction->setIcon(QIcon::fromTheme(QStringLiteral("system-software-update")));
    updateAction->setText(i18nc("@action Checks the Internet for updates", "Check for Updates"));
    updateAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_R));
    connect(this, &PackageKitBackend::fetchingChanged, updateAction, [updateAction, this](){
        updateAction->setEnabled(!isFetching());
    });
    connect(updateAction, &QAction::triggered, this, &PackageKitBackend::refreshDatabase);
    m_messageActions += updateAction;

    connect(PackageKit::Daemon::global(), &PackageKit::Daemon::updatesChanged, this, &PackageKitBackend::fetchUpdates);
    connect(PackageKit::Daemon::global(), &PackageKit::Daemon::isRunningChanged, this, &PackageKitBackend::checkDaemonRunning);
    connect(m_reviews, &AppstreamReviews::ratingsReady, this, &AbstractResourcesBackend::allDataChanged);

    SourcesModel::global()->addSourcesBackend(new PackageKitSourcesBackend(this));
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

    if (m_isFetching==0) {
        m_packages = m_updatingPackages;
        m_updatingPackages.clear();
    }

    if ((!f && m_isFetching==0) || (f && m_isFetching==1)) {
        emit fetchingChanged();
    }
    Q_ASSERT(m_isFetching>=0);
}

void PackageKitBackend::reloadPackageList()
{
    m_updatingPackages = m_packages;
    
    if (m_refresher) {
        disconnect(m_refresher.data(), &PackageKit::Transaction::finished, this, &PackageKitBackend::reloadPackageList);
    }

    const auto components = m_appdata.allComponents();
    QStringList neededPackages;
    neededPackages.reserve(components.size());
    foreach(const Appstream::Component& component, components) {
        if (component.packageNames().isEmpty()) {
            qDebug() << "no packages for" << component.name();
            continue;
        }
        neededPackages += component.packageNames();

        const auto res = new AppPackageKitResource(component, this);
        m_updatingPackages.packages[component.id()] = res;
        foreach (const QString& pkg, component.packageNames()) {
            m_updatingPackages.packageToApp[pkg] += component.id();
        }

        foreach (const QString& pkg, component.extends()) {
            m_updatingPackages.extendedBy[pkg] += res;
        }
    }
    neededPackages.removeDuplicates();

    PackageKit::Transaction * t = PackageKit::Daemon::resolve(neededPackages);
    connect(t, &PackageKit::Transaction::finished, this, &PackageKitBackend::getPackagesFinished);
    connect(t, &PackageKit::Transaction::package, this, &PackageKitBackend::addPackage);
    connect(t, &PackageKit::Transaction::errorCode, this, &PackageKitBackend::transactionError);

    fetchUpdates();
    acquireFetching(true);
}

void PackageKitBackend::fetchUpdates()
{
    m_updatesPackageId.clear();

    PackageKit::Transaction * tUpdates = PackageKit::Daemon::getUpdates();
    connect(tUpdates, &PackageKit::Transaction::finished, this, &PackageKitBackend::getUpdatesFinished);
    connect(tUpdates, &PackageKit::Transaction::package, this, &PackageKitBackend::addPackageToUpdate);
    connect(tUpdates, &PackageKit::Transaction::errorCode, this, &PackageKitBackend::transactionError);
    acquireFetching(true);
}


void PackageKitBackend::addPackage(PackageKit::Transaction::Info info, const QString &packageId, const QString &summary)
{
    const QString packageName = PackageKit::Daemon::packageName(packageId);
    QVector<AbstractResource*> r = resourcesByPackageName(packageName, true);
    if (r.isEmpty()) {
        r += new PackageKitResource(packageName, summary, this);
        m_updatingPackages.packages[packageName] = r.last();
    }
    foreach(auto res, r)
        static_cast<PackageKitResource*>(res)->addPackageId(info, packageId);
}

void PackageKitBackend::getPackagesFinished(PackageKit::Transaction::Exit exit)
{
    Q_ASSERT(m_isFetching);

    if (exit != PackageKit::Transaction::ExitSuccess) {
        qWarning() << "error while fetching details" << exit;
    }

    for(auto it = m_updatingPackages.packages.begin(); it != m_updatingPackages.packages.end(); ) {
        auto pkr = qobject_cast<PackageKitResource*>(it.value());
        if (pkr->packages().isEmpty()) {
            qWarning() << "Failed to find package for" << it.key();
            it.value()->deleteLater();
            it = m_updatingPackages.packages.erase(it);
        } else
            ++it;
    }

    acquireFetching(false);
}

void PackageKitBackend::transactionError(PackageKit::Transaction::Error, const QString& message)
{
    qWarning() << "Transaction error: " << message << sender();
}

void PackageKitBackend::packageDetails(const PackageKit::Details& details)
{
    QVector<AbstractResource*> resources = resourcesByPackageName(PackageKit::Daemon::packageName(details.packageId()), true);
    if (resources.isEmpty())
        qWarning() << "couldn't find package for" << details.packageId();

    foreach(AbstractResource* res, resources) {
        qobject_cast<PackageKitResource*>(res)->setDetails(details);
    }
}

QVector<AbstractResource*> PackageKitBackend::resourcesByPackageName(const QString& name, bool updating) const
{
    const Packages * const f = (updating ? &m_updatingPackages : &m_packages);
    const QHash<QString, QStringList> *dictionary = &f->packageToApp;
    const QHash<QString, AbstractResource*> *pkgs = &f->packages;

    const QStringList names = dictionary->value(name, QStringList(name));
    QVector<AbstractResource*> ret;
    ret.reserve(names.size());
    foreach(const QString& name, names) {
        AbstractResource* res = pkgs->value(name);
        if (res)
            ret += res;
    }
    return ret;
}

void PackageKitBackend::refreshDatabase()
{
    if (!m_refresher) {
        m_refresher = PackageKit::Daemon::refreshCache(false);
        connect(m_refresher.data(), &PackageKit::Transaction::finished, this, [this]() {
            reloadPackageList();
            acquireFetching(false);
            delete m_refresher;
        });
        acquireFetching(true);
    } else {
        qWarning() << "already resetting";
    }
}

QVector<AbstractResource*> PackageKitBackend::allResources() const
{
    return containerValues<QVector<AbstractResource*>>(m_packages.packages);
}

AbstractResource* PackageKitBackend::resourceByPackageName(const QString& name) const
{
    const QStringList ids = m_packages.packageToApp.value(name, QStringList(name));
    return ids.isEmpty() ? nullptr : m_packages.packages[ids.first()];
}

QList<AbstractResource*> PackageKitBackend::searchPackageName(const QString& searchText)
{
    QList<AbstractResource*> ret;
    Q_FOREACH (AbstractResource* res, m_packages.packages) {
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

void PackageKitBackend::transactionCanceled(Transaction* t)
{
    qDebug() << "Cancel transaction:" << t->resource()->packageName() << "with" << m_transactions.size() << "transactions running";
    int count = m_transactions.removeAll(t);
    Q_ASSERT(count==1);
    Q_UNUSED(count)
    TransactionModel::global()->cancelTransaction(t);
}

void PackageKitBackend::removeTransaction(Transaction* t)
{
    qDebug() << "Remove transaction:" << t->resource()->packageName() << "with" << m_transactions.size() << "transactions running";
    int count = m_transactions.removeAll(t);
    Q_ASSERT(count==1);
    Q_UNUSED(count)
    TransactionModel::global()->removeTransaction(t);
}

void PackageKitBackend::addTransaction(PKTransaction* t)
{
    m_transactions.append(t);
    TransactionModel::global()->addTransaction(t);
    t->start();
}

void PackageKitBackend::installApplication(AbstractResource* app, const AddonList& addons)
{
    if(!addons.addonsToInstall().isEmpty())
    {
        QVector<AbstractResource*> appsToInstall;

        if(!app->isInstalled())
            appsToInstall << app;

        foreach(const QString& toInstall, addons.addonsToInstall()) {
            appsToInstall += m_packages.packages.value(toInstall);
            Q_ASSERT(appsToInstall.last());
        }
        addTransaction(new PKTransaction(appsToInstall, Transaction::ChangeAddonsRole));
    }

    if (!addons.addonsToRemove().isEmpty()) {
        QVector<AbstractResource*> appsToRemove;
        foreach(const QString& toRemove, addons.addonsToRemove()) {
            appsToRemove += m_packages.packages.value(toRemove);
        }
        addTransaction(new PKTransaction(appsToRemove, Transaction::RemoveRole));
    }
}

void PackageKitBackend::installApplication(AbstractResource* app)
{
    addTransaction(new PKTransaction({app}, Transaction::InstallRole));
}

void PackageKitBackend::cancelTransaction(AbstractResource* app)
{
    Q_FOREACH (Transaction* t, m_transactions) {
        PKTransaction* pkt = qobject_cast<PKTransaction*>(t);
        if (pkt->resource() == app) {
            if (pkt->transaction()->allowCancel()) {
                pkt->transaction()->cancel();
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
    addTransaction(new PKTransaction({app}, Transaction::RemoveRole));
}

QList<AbstractResource*> PackageKitBackend::upgradeablePackages() const
{
    QVector<AbstractResource*> ret;
    ret.reserve(m_updatesPackageId.size());
    Q_FOREACH (const QString& pkgid, m_updatesPackageId) {
        const QString pkgname = PackageKit::Daemon::packageName(pkgid);
        const auto pkgs = resourcesByPackageName(pkgname, false);
        if (pkgs.isEmpty()) {
            qWarning() << "couldn't find resource for" << pkgid;
        }
        ret += pkgs;
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
        connect(transaction, &PackageKit::Transaction::details, this, &PackageKitBackend::packageDetails);
        connect(transaction, &PackageKit::Transaction::errorCode, this, &PackageKitBackend::transactionError);
        connect(transaction, &PackageKit::Transaction::finished, this, &PackageKitBackend::getUpdatesDetailsFinished);
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

bool PackageKitBackend::isPackageNameUpgradeable(const PackageKitResource* res) const
{
    return !upgradeablePackageId(res).isEmpty();
}

QString PackageKitBackend::upgradeablePackageId(const PackageKitResource* res) const
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

QVector<AppPackageKitResource*> PackageKitBackend::extendedBy(const QString& id) const
{
    return m_packages.extendedBy[id];
}

AbstractReviewsBackend* PackageKitBackend::reviewsBackend() const
{
    return m_reviews;
}

#include "PackageKitBackend.moc"
