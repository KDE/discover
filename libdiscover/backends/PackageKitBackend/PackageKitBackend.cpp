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
#include "LocalFilePKResource.h"
#include "AppstreamReviews.h"
#include <resources/AbstractResource.h>
#include <resources/StandardBackendUpdater.h>
#include <resources/SourcesModel.h>
#include <Transaction/TransactionModel.h>

#include <QProcess>
#include <QStringList>
#include <QDebug>
#include <QTimer>
#include <QStandardPaths>

#include <PackageKit/Transaction>
#include <PackageKit/Daemon>
#include <PackageKit/Details>

#include <KDesktopFile>
#include <KLocalizedString>
#include <QAction>

#include "utils.h"
#include "config-paths.h"

MUON_BACKEND_PLUGIN(PackageKitBackend)

static QString locateService(const QString &filename)
{
    return QStandardPaths::locate(QStandardPaths::GenericDataLocation, QStringLiteral("applications/")+filename);
}

PackageKitBackend::PackageKitBackend(QObject* parent)
    : AbstractResourcesBackend(parent)
    , m_updater(new PackageKitUpdater(this))
    , m_refresher(nullptr)
    , m_isFetching(0)
    , m_reviews(new AppstreamReviews(this))
{
    bool b = m_appdata.load();
    if (!b) {
        qWarning() << "Could not open the AppStream metadata pool";

        QTimer::singleShot(0, this, [this]() {
            Q_EMIT passiveMessage(i18n("Please make sure that Appstream is properly set up on your system"));
        });
    }
    reloadPackageList();

    QTimer* t = new QTimer(this);
    connect(t, &QTimer::timeout, this, &PackageKitBackend::refreshDatabase);
    t->setInterval(60 * 60 * 1000);
    t->setSingleShot(false);
    t->start();

    m_delayedDetailsFetch.setSingleShot(true);
    m_delayedDetailsFetch.setInterval(0);
    connect(&m_delayedDetailsFetch, &QTimer::timeout, this, &PackageKitBackend::performDetailsFetch);

    QAction* updateAction = new QAction(this);
    updateAction->setIcon(QIcon::fromTheme(QStringLiteral("system-software-update")));
    updateAction->setText(i18nc("@action Checks the Internet for updates", "Check for Updates"));
    updateAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_R));
    connect(this, &PackageKitBackend::fetchingChanged, updateAction, [updateAction, this](){
        updateAction->setEnabled(!isFetching());
    });
    connect(updateAction, &QAction::triggered, this, &PackageKitBackend::refreshDatabase);
    m_messageActions += updateAction;

    const auto service = locateService(QStringLiteral("software-properties-kde.desktop"));
    if (!service.isEmpty())
        m_messageActions += createActionForService(service);

    connect(PackageKit::Daemon::global(), &PackageKit::Daemon::updatesChanged, this, &PackageKitBackend::fetchUpdates);
    connect(PackageKit::Daemon::global(), &PackageKit::Daemon::isRunningChanged, this, &PackageKitBackend::checkDaemonRunning);
    connect(m_reviews, &AppstreamReviews::ratingsReady, this, &AbstractResourcesBackend::emitRatingsReady);

    SourcesModel::global()->addSourcesBackend(new PackageKitSourcesBackend(this));
}

PackageKitBackend::~PackageKitBackend()
{
}

QAction* PackageKitBackend::createActionForService(const QString &servicePath)
{
    QAction* action = new QAction(this);
    KDesktopFile parser(servicePath);
    action->setIcon(QIcon::fromTheme(parser.readIcon()));
    action->setText(parser.readName());
    connect(action, &QAction::triggered, action, [servicePath, this](){
        bool b = QProcess::startDetached(QStringLiteral(CMAKE_INSTALL_FULL_LIBEXECDIR_KF5 "/discover/runservice"), {servicePath});
        if (!b)
            qWarning() << "Could not start" << servicePath;
    });
    return action;
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

    const auto components = m_appdata.components();
    QStringList neededPackages;
    neededPackages.reserve(components.size());
    foreach(const AppStream::Component& component, components) {
        const auto pkgNames = component.packageNames();
        if (pkgNames.isEmpty()) {
            if (component.kind() == AppStream::Component::KindDesktopApp) {
                const QString file = locateService(component.desktopId());
                if (!file.isEmpty()) {
                    auto trans = PackageKit::Daemon::searchFiles(file);
                    connect(trans, &PackageKit::Transaction::package, this, [trans](PackageKit::Transaction::Info info, const QString &packageID){
                        if (info == PackageKit::Transaction::InfoInstalled)
                            trans->setProperty("installedPackage", packageID);
                    });
                    connect(trans, &PackageKit::Transaction::finished, this, [this, trans, component](PackageKit::Transaction::Exit status) {
                        const auto pkgidVal = trans->property("installedPackage");
                        if (status == PackageKit::Transaction::ExitSuccess && !pkgidVal.isNull()) {
                            const auto pkgid = pkgidVal.toString();
                            acquireFetching(true);
                            auto res = addComponent(component, {PackageKit::Daemon::packageName(pkgid)});
                            res->addPackageId(PackageKit::Transaction::InfoInstalled, pkgid, true);
                            acquireFetching(false);
                        }
                    });
                    continue;
                }
            }

            qDebug() << "no packages for" << component.name();
            continue;
        }
        neededPackages += pkgNames;

        addComponent(component, pkgNames);
    }

    acquireFetching(false);
    neededPackages.removeDuplicates();
    resolvePackages(neededPackages);
}

AppPackageKitResource* PackageKitBackend::addComponent(const AppStream::Component& component, const QStringList& pkgNames)
{
    Q_ASSERT(isFetching());
    Q_ASSERT(!pkgNames.isEmpty());

    const auto res = new AppPackageKitResource(component, pkgNames.at(0), this);
    m_packages.packages[component.id()] = res;
    foreach (const QString& pkg, pkgNames) {
        m_packages.packageToApp[pkg] += component.id();
    }

    foreach (const QString& pkg, component.extends()) {
        m_packages.extendedBy[pkg] += res;
    }
    return res;
}

class TransactionSet : public QObject
{
    Q_OBJECT
    public:
        TransactionSet(const QVector<PackageKit::Transaction*> &transactions)
            : m_transactions(transactions)
        {
            foreach(PackageKit::Transaction* t, transactions) {
                connect(t, &PackageKit::Transaction::finished, this, &TransactionSet::transactionFinished);
            }
        }

        void transactionFinished(PackageKit::Transaction::Exit exit) {
            PackageKit::Transaction* t = qobject_cast<PackageKit::Transaction*>(sender());
            if (exit != PackageKit::Transaction::ExitSuccess) {
                qWarning() << "failed" << exit << t;
            }

            m_transactions.removeAll(t);
            if (m_transactions.isEmpty()) {
                Q_EMIT allFinished();
            }
        }

    Q_SIGNALS:
        void allFinished();

    private:
        QVector<PackageKit::Transaction*> m_transactions;

};

void PackageKitBackend::clearPackages(const QStringList& packageNames)
{
    const auto resources = resourcesByPackageNames<QVector<AbstractResource*>>(packageNames);
    for(auto res: resources) {
        qobject_cast<PackageKitResource*>(res)->clearPackageIds();
    }
}

void PackageKitBackend::resolvePackages(const QStringList &packageNames)
{
    PackageKit::Transaction * tArch = PackageKit::Daemon::resolve(packageNames, PackageKit::Transaction::FilterArch);
    connect(tArch, &PackageKit::Transaction::package, this, &PackageKitBackend::addPackageArch);
    connect(tArch, &PackageKit::Transaction::errorCode, this, &PackageKitBackend::transactionError);

    PackageKit::Transaction * tNotArch = PackageKit::Daemon::resolve(packageNames, PackageKit::Transaction::FilterNotArch);
    connect(tNotArch, &PackageKit::Transaction::package, this, &PackageKitBackend::addPackageNotArch);
    connect(tNotArch, &PackageKit::Transaction::errorCode, this, &PackageKitBackend::transactionError);

    TransactionSet* merge = new TransactionSet({tArch, tNotArch});
    connect(merge, &TransactionSet::allFinished, this, &PackageKitBackend::getPackagesFinished);
    fetchUpdates();
}

void PackageKitBackend::fetchUpdates()
{
    PackageKit::Transaction * tUpdates = PackageKit::Daemon::getUpdates();
    connect(tUpdates, &PackageKit::Transaction::finished, this, &PackageKitBackend::getUpdatesFinished);
    connect(tUpdates, &PackageKit::Transaction::package, this, &PackageKitBackend::addPackageToUpdate);
    connect(tUpdates, &PackageKit::Transaction::errorCode, this, &PackageKitBackend::transactionError);
    m_updatesPackageId.clear();

    m_updater->setProgressing(true);
}

void PackageKitBackend::addPackageArch(PackageKit::Transaction::Info info, const QString& packageId, const QString& summary)
{
    addPackage(info, packageId, summary, true);
}

void PackageKitBackend::addPackageNotArch(PackageKit::Transaction::Info info, const QString& packageId, const QString& summary)
{
    addPackage(info, packageId, summary, false);
}

void PackageKitBackend::addPackage(PackageKit::Transaction::Info info, const QString &packageId, const QString &summary, bool arch)
{
    const QString packageName = PackageKit::Daemon::packageName(packageId);
    QSet<AbstractResource*> r = resourcesByPackageName(packageName);
    if (r.isEmpty()) {
        auto pk = new PackageKitResource(packageName, summary, this);
        r = { pk };
        m_packagesToAdd.insert(pk);
    }
    foreach(auto res, r)
        static_cast<PackageKitResource*>(res)->addPackageId(info, packageId, arch);
}

void PackageKitBackend::getPackagesFinished()
{
    for(auto it = m_packages.packages.cbegin(); it != m_packages.packages.cend(); ++it) {
        auto pkr = qobject_cast<PackageKitResource*>(it.value());
        if (pkr->packages().isEmpty()) {
            qWarning() << "Failed to find package for" << it.key();
            m_packagesToDelete += pkr;
        }
    }
    includePackagesToAdd();
}

void PackageKitBackend::includePackagesToAdd()
{
    if (m_packagesToAdd.isEmpty() && m_packagesToDelete.isEmpty())
        return;

    acquireFetching(true);
    foreach(PackageKitResource* res, m_packagesToAdd) {
        m_packages.packages[res->packageName()] = res;
    }
    foreach(PackageKitResource* res, m_packagesToDelete) {
        const auto pkgs = m_packages.packageToApp.value(res->packageName(), {res->packageName()});
        foreach(const auto &pkg, pkgs) {
            auto res = m_packages.packages.take(pkg);
            emit resourceRemoved(res);
            res->deleteLater();
        }
    }
    m_packagesToAdd.clear();
    m_packagesToDelete.clear();
    acquireFetching(false);
}

void PackageKitBackend::transactionError(PackageKit::Transaction::Error, const QString& message)
{
    qWarning() << "Transaction error: " << message << sender();
    Q_EMIT passiveMessage(message);
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
    return resourcesByPackageNames<QSet<AbstractResource*>>({name});
}

template <typename T>
T PackageKitBackend::resourcesByPackageNames(const QStringList &pkgnames) const
{
    T ret;
    ret.reserve(pkgnames.size());
    for(const QString &name : pkgnames) {
        const QStringList names = m_packages.packageToApp.value(name, QStringList(name));
        foreach(const QString& name, names) {
            AbstractResource* res = m_packages.packages.value(name);
            if (res)
                ret += res;
        }
    }
    return ret;
}

void PackageKitBackend::refreshDatabase()
{
    if (!m_refresher) {
        acquireFetching(true);
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

ResultsStream* PackageKitBackend::search(const AbstractResourcesBackend::Filters& filter)
{
    if (filter.search.isEmpty()) {
        return new ResultsStream(QStringLiteral("PackageKitStream"), kFilter<QVector<AbstractResource*>>(m_packages.packages, [](AbstractResource* res) { return !res->isTechnical(); }));
    } else {
        const QList<AppStream::Component> components = m_appdata.search(filter.search);
        const QStringList ids = kTransform<QStringList>(components, [](const AppStream::Component& comp) { return comp.id(); });
        auto stream = new ResultsStream(QStringLiteral("PackageKitStream"));
        if (!ids.isEmpty()) {
            const auto resources = resourcesByPackageNames<QVector<AbstractResource*>>(ids);
            QTimer::singleShot(0, this, [stream, resources, this] () {
                stream->resourcesFound(resources);
            });
        }

        PackageKit::Transaction * tArch = PackageKit::Daemon::resolve(filter.search, PackageKit::Transaction::FilterArch);
        connect(tArch, &PackageKit::Transaction::package, this, &PackageKitBackend::addPackageArch);
        connect(tArch, &PackageKit::Transaction::package, this, [tArch](PackageKit::Transaction::Info /*info*/, const QString &packageId){
            tArch->setProperty("packageId", packageId);
        });
        connect(tArch, &PackageKit::Transaction::finished, this, [stream, tArch, ids, this]() {
            getPackagesFinished();
            const auto packageId = tArch->property("packageId");
            if (!packageId.isNull()) {
                const auto res = resourcesByPackageNames<QVector<AbstractResource*>>({PackageKit::Daemon::packageName(packageId.toString())});
                stream->resourcesFound(kFilter<QVector<AbstractResource*>>(res, [ids](AbstractResource* res){ return !ids.contains(res->appstreamId()); }));
            }
            stream->deleteLater();
        });
        return stream;
    }
}

ResultsStream * PackageKitBackend::findResourceByPackageName(const QString& search)
{
    auto pkg = m_packages.packages.value(search);
    return new ResultsStream(QStringLiteral("PackageKitStream"), pkg ? QVector<AbstractResource*>{pkg} : QVector<AbstractResource*>{});
}

int PackageKitBackend::updatesCount() const
{
    return m_updatesPackageId.count();
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
        new PKTransaction(appsToInstall, Transaction::ChangeAddonsRole);
    }

    if (!addons.addonsToRemove().isEmpty()) {
        QVector<AbstractResource*> appsToRemove = kTransform<QVector<AbstractResource*>>(addons.addonsToRemove(), [this](const QString& toRemove){ return m_packages.packages.value(toRemove); });
        new PKTransaction(appsToRemove, Transaction::RemoveRole);
    }

    if (!app->isInstalled())
        installApplication(app);
}

void PackageKitBackend::installApplication(AbstractResource* app)
{
    new PKTransaction({app}, Transaction::InstallRole);
}

void PackageKitBackend::removeApplication(AbstractResource* app)
{
    Q_ASSERT(!isFetching());
    new PKTransaction({app}, Transaction::RemoveRole);
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
        addPackage(info, packageId, summary, true);
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

    m_updater->setProgressing(false);

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

void PackageKitBackend::fetchDetails(const QString& pkgid)
{
    if (!m_delayedDetailsFetch.isActive()) {
        m_delayedDetailsFetch.start();
    }

    m_packageNamesToFetchDetails += pkgid;
}

void PackageKitBackend::performDetailsFetch()
{
    Q_ASSERT(!m_packageNamesToFetchDetails.isEmpty());
    PackageKit::Transaction* transaction = PackageKit::Daemon::getDetails(m_packageNamesToFetchDetails.toList());
    connect(transaction, &PackageKit::Transaction::details, this, &PackageKitBackend::packageDetails);
    connect(transaction, &PackageKit::Transaction::errorCode, this, &PackageKitBackend::transactionError);
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

AbstractResource * PackageKitBackend::resourceForFile(const QUrl& file)
{
    return new LocalFilePKResource(file, this);
}


#include "PackageKitBackend.moc"
