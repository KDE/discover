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
    connect(m_reviews, &AppstreamReviews::ratingsReady, this, &AbstractResourcesBackend::emitRatingsReady);

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

    if ((!f && m_isFetching==0) || (f && m_isFetching==1)) {
        emit fetchingChanged();
    }
    Q_ASSERT(m_isFetching>=0);
}

void PackageKitBackend::reloadPackageList()
{
    acquireFetching(true);
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
        m_packages.packages[component.id()] = res;
        foreach (const QString& pkg, component.packageNames()) {
            m_packages.packageToApp[pkg] += component.id();
        }

        foreach (const QString& pkg, component.extends()) {
            m_packages.extendedBy[pkg] += res;
        }
    }
    acquireFetching(false);
    neededPackages.removeDuplicates();

    PackageKit::Transaction * t = PackageKit::Daemon::resolve(neededPackages);
    connect(t, &PackageKit::Transaction::finished, this, &PackageKitBackend::getPackagesFinished);
    connect(t, &PackageKit::Transaction::package, this, &PackageKitBackend::addPackage);
    connect(t, &PackageKit::Transaction::errorCode, this, &PackageKitBackend::transactionError);

    fetchUpdates();
}

void PackageKitBackend::fetchUpdates()
{
    PackageKit::Transaction * tUpdates = PackageKit::Daemon::getUpdates();
    connect(tUpdates, &PackageKit::Transaction::finished, this, &PackageKitBackend::getUpdatesFinished);
    connect(tUpdates, &PackageKit::Transaction::package, this, &PackageKitBackend::addPackageToUpdate);
    connect(tUpdates, &PackageKit::Transaction::errorCode, this, &PackageKitBackend::transactionError);
    m_updatesPackageId.clear();
}


void PackageKitBackend::addPackage(PackageKit::Transaction::Info info, const QString &packageId, const QString &summary)
{
    const QString packageName = PackageKit::Daemon::packageName(packageId);
    QSet<AbstractResource*> r = resourcesByPackageName(packageName);
    if (r.isEmpty()) {
        auto pk = new PackageKitResource(packageName, summary, this);
        r = { pk };
        m_packagesToAdd.insert(pk);
    }
    foreach(auto res, r)
        static_cast<PackageKitResource*>(res)->addPackageId(info, packageId);
}

void PackageKitBackend::getPackagesFinished(PackageKit::Transaction::Exit exit)
{
    if (exit != PackageKit::Transaction::ExitSuccess) {
        qWarning() << "error while fetching details" << exit;
    }

    for(auto it = m_packages.packages.begin(); it != m_packages.packages.end(); ) {
        auto pkr = qobject_cast<PackageKitResource*>(it.value());
        if (pkr->packages().isEmpty()) {
            qWarning() << "Failed to find package for" << it.key();
            it.value()->deleteLater();
            it = m_packages.packages.erase(it);
        } else
            ++it;
    }
    includePackagesToAdd();
}

void PackageKitBackend::includePackagesToAdd()
{
    if (m_packagesToAdd.isEmpty())
        return;

    acquireFetching(true);
    foreach(PackageKitResource* res, m_packagesToAdd) {
        m_packages.packages[res->packageName()] = res;
    }
    acquireFetching(false);
}

void PackageKitBackend::transactionError(PackageKit::Transaction::Error, const QString& message)
{
    qWarning() << "Transaction error: " << message << sender();
}

void PackageKitBackend::packageDetails(const PackageKit::Details& details)
{
    const QSet<AbstractResource*> resources = resourcesByPackageName(PackageKit::Daemon::packageName(details.packageId()));
    if (resources.isEmpty())
        qWarning() << "couldn't find package for" << details.packageId();

    foreach(AbstractResource* res, resources) {
        qobject_cast<PackageKitResource*>(res)->setDetails(details);
    }
}

QSet<AbstractResource*> PackageKitBackend::resourcesByPackageName(const QString& name) const
{
    const QStringList names = m_packages.packageToApp.value(name, QStringList(name));
    QSet<AbstractResource*> ret;
    ret.reserve(names.size());
    foreach(const QString& name, names) {
        AbstractResource* res = m_packages.packages.value(name);
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

void PackageKitBackend::removeApplication(AbstractResource* app)
{
    Q_ASSERT(!isFetching());
    addTransaction(new PKTransaction({app}, Transaction::RemoveRole));
}

QSet<AbstractResource*> PackageKitBackend::upgradeablePackages() const
{
    QSet<AbstractResource*> ret;
    ret.reserve(m_updatesPackageId.size());
    Q_FOREACH (const QString& pkgid, m_updatesPackageId) {
        const QString pkgname = PackageKit::Daemon::packageName(pkgid);
        const auto pkgs = resourcesByPackageName(pkgname);
        if (pkgs.isEmpty()) {
            qWarning() << "couldn't find resource for" << pkgid;
        }
        ret.unite(pkgs);
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
    if (!m_updatesPackageId.isEmpty()) {
        PackageKit::Transaction* transaction = PackageKit::Daemon::getDetails(m_updatesPackageId.toList());
        connect(transaction, &PackageKit::Transaction::details, this, &PackageKitBackend::packageDetails);
        connect(transaction, &PackageKit::Transaction::errorCode, this, &PackageKitBackend::transactionError);
        connect(transaction, &PackageKit::Transaction::finished, this, &PackageKitBackend::getUpdatesDetailsFinished);
    }

    includePackagesToAdd();
    emit updatesCountChanged();
}

void PackageKitBackend::getUpdatesDetailsFinished(PackageKit::Transaction::Exit exit, uint)
{
    if (exit != PackageKit::Transaction::ExitSuccess) {
        qWarning() << "Couldn't figure out the updates on PackageKit backend" << exit;
    }
}

bool PackageKitBackend::isPackageNameUpgradeable(const PackageKitResource* res) const
{
    return !upgradeablePackageId(res).isEmpty();
}

QString PackageKitBackend::upgradeablePackageId(const PackageKitResource* res) const
{
    QString name = res->packageName();
    foreach (const QString& pkgid, m_updatesPackageId) {
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
