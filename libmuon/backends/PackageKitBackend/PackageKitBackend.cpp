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
#include "AppstreamUtils.h"
#include "PKTransaction.h"
#include <resources/AbstractResource.h>
#include <resources/StandardBackendUpdater.h>
#include <Transaction/TransactionModel.h>
#include <QStringList>
#include <QDebug>
#include <QTimer>
#include <QTimerEvent>
#include <PackageKit/packagekit-qt2/Transaction>
#include <PackageKit/packagekit-qt2/Daemon>

#include <KPluginFactory>
#include <KLocalizedString>
#include <KAboutData>
#include <KDebug>

K_PLUGIN_FACTORY(MuonPackageKitBackendFactory, registerPlugin<PackageKitBackend>(); )
K_EXPORT_PLUGIN(MuonPackageKitBackendFactory(KAboutData("muon-pkbackend","muon-pkbackend",ki18n("PackageKit Backend"),"0.1",ki18n("Install PackageKit data in your system"), KAboutData::License_GPL)))

QString PackageKitBackend::errorMessage(PackageKit::Transaction::Error error)
{
    switch(error) {
        case PackageKit::Transaction::ErrorOom:
            return QString();//FIXME: What error is this?!
        case PackageKit::Transaction::ErrorNoNetwork:
            return i18n("No network connection available!");
        case PackageKit::Transaction::ErrorNotSupported:
            return i18n("Operation not supported!");
        case PackageKit::Transaction::ErrorInternalError:
            return i18n("Internal error!");
        case PackageKit::Transaction::ErrorGpgFailure:
            return i18n("GPG failure!");
        case PackageKit::Transaction::ErrorPackageIdInvalid:
            return i18n("PackageID invalid!");
        case PackageKit::Transaction::ErrorPackageNotInstalled:
            return i18n("Package not installed!");
        case PackageKit::Transaction::ErrorPackageNotFound:
            return i18n("Package not found!");
        case PackageKit::Transaction::ErrorPackageAlreadyInstalled:
            return i18n("Package is already installed!");
        case PackageKit::Transaction::ErrorPackageDownloadFailed:
            return i18n("Package download failed!");
        case PackageKit::Transaction::ErrorGroupNotFound:
            return i18n("Package group not found!");
        case PackageKit::Transaction::ErrorGroupListInvalid:
            return i18n("Package group list invalid!");
        case PackageKit::Transaction::ErrorDepResolutionFailed:
            return i18n("Dependency resolution failed!");
        case PackageKit::Transaction::ErrorFilterInvalid:
            return i18n("Filter invalid!");
        case PackageKit::Transaction::ErrorCreateThreadFailed:
            return i18n("Failed while creating a thread!");
        case PackageKit::Transaction::ErrorTransactionError:
            return i18n("Transaction failure!");
        case PackageKit::Transaction::ErrorTransactionCancelled:
            return i18n("Transaction cancelled!");
        case PackageKit::Transaction::ErrorNoCache:
            return i18n("No Cache available");
        case PackageKit::Transaction::ErrorRepoNotFound:
            return i18n("Cannot find repository!");
        case PackageKit::Transaction::ErrorCannotRemoveSystemPackage:
            return i18n("Cannot remove system package!");
        case PackageKit::Transaction::ErrorProcessKill:
            return i18n("Cannot kill process!");
        case PackageKit::Transaction::ErrorFailedInitialization:
            return i18n("Initialization failure!");
        case PackageKit::Transaction::ErrorFailedFinalise:
            return i18n("Failed to finalize transaction!");
        case PackageKit::Transaction::ErrorFailedConfigParsing:
            return i18n("Config parsing failed!");
        case PackageKit::Transaction::ErrorCannotCancel:
            return i18n("Cannot cancel transaction");
        case PackageKit::Transaction::ErrorCannotGetLock:
            return i18n("Cannot obtain lock!");
        case PackageKit::Transaction::ErrorNoPackagesToUpdate:
            return i18n("No packages to update!");
        case PackageKit::Transaction::ErrorCannotWriteRepoConfig:
            return i18n("Cannot write repo config!");
        case PackageKit::Transaction::ErrorLocalInstallFailed:
            return i18n("Local install failed!");
        case PackageKit::Transaction::ErrorBadGpgSignature:
            return i18n("Bad GPG signature found!");
        case PackageKit::Transaction::ErrorMissingGpgSignature:
            return i18n("No GPG signature found!");
        case PackageKit::Transaction::ErrorCannotInstallSourcePackage:
            return i18n("Cannot install source package!");
        case PackageKit::Transaction::ErrorRepoConfigurationError:
            return i18n("Repo configuration error!");
        case PackageKit::Transaction::ErrorNoLicenseAgreement:
            return i18n("No license agreement!");
        case PackageKit::Transaction::ErrorFileConflicts:
            return i18n("File conflicts found!");
        case PackageKit::Transaction::ErrorPackageConflicts:
            return i18n("Package conflict found!");
        case PackageKit::Transaction::ErrorRepoNotAvailable:
            return i18n("Repo not available!");
        case PackageKit::Transaction::ErrorInvalidPackageFile:
            return i18n("Invalid package file!");
        case PackageKit::Transaction::ErrorPackageInstallBlocked:
            return i18n("Package install blocked!");
        case PackageKit::Transaction::ErrorPackageCorrupt:
            return i18n("Corrupt package found!");
        case PackageKit::Transaction::ErrorAllPackagesAlreadyInstalled:
            return i18n("All packages already installed!");
        case PackageKit::Transaction::ErrorFileNotFound:
            return i18n("File not found!");
        case PackageKit::Transaction::ErrorNoMoreMirrorsToTry:
            return i18n("No more mirrors available!");
        case PackageKit::Transaction::ErrorNoDistroUpgradeData:
            return i18n("No distro upgrade data!");
        case PackageKit::Transaction::ErrorIncompatibleArchitecture:
            return i18n("Incompatible architecture!");
        case PackageKit::Transaction::ErrorNoSpaceOnDevice:
            return i18n("No space on device left!");
        case PackageKit::Transaction::ErrorMediaChangeRequired:
            return i18n("A media change is required!");
        case PackageKit::Transaction::ErrorNotAuthorized:
            return i18n("You have no authorization to execute this operation!");
        case PackageKit::Transaction::ErrorUpdateNotFound:
            return i18n("Update not found!");
        case PackageKit::Transaction::ErrorCannotInstallRepoUnsigned:
            return  i18n("Cannot install from unsigned repo!");
        case PackageKit::Transaction::ErrorCannotUpdateRepoUnsigned:
            return i18n("Cannot update from unsigned repo!");
        case PackageKit::Transaction::ErrorCannotGetFilelist:
            return i18n("Cannot get file list!");
        case PackageKit::Transaction::ErrorCannotGetRequires:
            return i18n("Cannot get requires!");
        case PackageKit::Transaction::ErrorCannotDisableRepository:
            return i18n("Cannot disable repository!");
        case PackageKit::Transaction::ErrorRestrictedDownload:
            return i18n("Restricted download detected!");
        case PackageKit::Transaction::ErrorPackageFailedToConfigure:
            return i18n("Package failed to configure!");
        case PackageKit::Transaction::ErrorPackageFailedToBuild:
            return i18n("Package failed to build!");
        case PackageKit::Transaction::ErrorPackageFailedToInstall:
            return i18n("Package failed to install!");
        case PackageKit::Transaction::ErrorPackageFailedToRemove:
            return i18n("Package failed to remove!");
        case PackageKit::Transaction::ErrorUpdateFailedDueToRunningProcess:
            return i18n("Update failed due to running process!");
        case PackageKit::Transaction::ErrorPackageDatabaseChanged:
            return i18n("The package database changed!");
        case PackageKit::Transaction::ErrorProvideTypeNotSupported:
            return i18n("The provided type is not supported!");
        case PackageKit::Transaction::ErrorInstallRootInvalid:
            return i18n("Install root is invalid!");
        case PackageKit::Transaction::ErrorCannotFetchSources:
            return i18n("Cannot fetch sources!");
        case PackageKit::Transaction::ErrorCancelledPriority:
            return i18n("Cancelled priority!");
        case PackageKit::Transaction::ErrorUnfinishedTransaction:
            return i18n("Unfinished transaction!");
        case PackageKit::Transaction::ErrorLockRequired:
            return i18n("Lock required!");
        case PackageKit::Transaction::ErrorUnknown:
        default:
            return i18n("Unknown error");
    }
}

PackageKitBackend::PackageKitBackend(QObject* parent, const QVariantList&)
    : AbstractResourcesBackend(parent)
    , m_updater(new PackageKitUpdater(this))
    , m_refresher(0)
{
    populateInstalledCache();
    emit backendReady();
    
    startTimer(60 * 60 * 1000);//Update database every 60 minutes
}

PackageKitBackend::~PackageKitBackend()
{
}

void PackageKitBackend::populateInstalledCache()
{
    kDebug() << "Starting to populate the installed packages cache";
    m_appdata = AppstreamUtils::fetchAppData("/home/lukas/appdata.xml");//FIXME: Change path
    
    emit reloadStarted();
    
    foreach (const ApplicationData &data, m_appdata.values()) {
        if (!data.pkgname.isEmpty())
            m_packages[data.pkgname] = new AppPackageKitResource(QString(), PackageKit::Transaction::InfoUnknown, QString(), data, this);
    }
    
    emit reloadFinished();
    
    m_updatingPackages = m_packages;
    
    if (m_refresher) {
        disconnect(m_refresher, SIGNAL(changed()), this, SLOT(populateInstalledCache()));
    }
    
    PackageKit::Transaction * t = new PackageKit::Transaction(this);
    
    connect(t, SIGNAL(finished(PackageKit::Transaction::Exit,uint)), this, SLOT(populateNewestCache()));
    connect(t, SIGNAL(package(PackageKit::Transaction::Info, QString, QString)), SLOT(addPackage(PackageKit::Transaction::Info, QString, QString)));
    connect(t, SIGNAL(destroy()), t, SLOT(deleteLater()));
    
    t->getPackages(PackageKit::Transaction::FilterInstalled | PackageKit::Transaction::FilterArch | PackageKit::Transaction::FilterLast);
    
}

void PackageKitBackend::addPackage(PackageKit::Transaction::Info info, const QString &packageId, const QString &summary)
{
    if (m_updatingPackages[PackageKit::Daemon::global()->packageName(packageId)]) {
        qobject_cast<PackageKitResource*>(m_updatingPackages[PackageKit::Daemon::global()->packageName(packageId)])->addPackageId(info, packageId, summary);
    } else {
        PackageKitResource* newResource = 0;
        QHash<QString, ApplicationData>::const_iterator it = m_appdata.constFind(PackageKit::Daemon::global()->packageName(packageId));
        if (it!=m_appdata.constEnd())
            newResource = new AppPackageKitResource(packageId, info, summary, *it, this);
        else
            newResource = new PackageKitResource(packageId, info, summary, this);
        m_updatingPackages[PackageKit::Daemon::global()->packageName(packageId)] = newResource;
    }
}

void PackageKitBackend::populateNewestCache()
{
    kDebug() << "Starting to populate the cache with newest packages";
    PackageKit::Transaction * t = new PackageKit::Transaction(this);
    
    connect(t, SIGNAL(destroy()), t, SLOT(deleteLater()));
    connect(t, SIGNAL(finished(PackageKit::Transaction::Exit,uint)), this, SLOT(finishRefresh()));
    connect(t, SIGNAL(package(PackageKit::Transaction::Info, QString, QString)), SLOT(addNewest(PackageKit::Transaction::Info, QString, QString)));
    
    t->getPackages(PackageKit::Transaction::FilterNewest | PackageKit::Transaction::FilterArch | PackageKit::Transaction::FilterLast);
}

void PackageKitBackend::addNewest(PackageKit::Transaction::Info info, const QString &packageId, const QString &summary)
{
    if (m_updatingPackages[PackageKit::Daemon::global()->packageName(packageId)]) {
        PackageKitResource * res = qobject_cast<PackageKitResource*>(m_updatingPackages[PackageKit::Daemon::global()->packageName(packageId)]);
        res->addPackageId(info, packageId, summary);
    } else {
        addPackage(info, packageId, summary);
    }
}

void PackageKitBackend::finishRefresh()
{
    kDebug() << "Finished the refresh and resetting packages" << m_updatingPackages.count();
    emit reloadStarted();
    
    m_packages = m_updatingPackages;
    
    emit reloadFinished();
}

void PackageKitBackend::timerEvent(QTimerEvent * event)
{
    updateDatabase();
}

void PackageKitBackend::updateDatabase()
{
    kDebug() << "Starting to update the package cache";
    if (!m_refresher) {
        m_refresher = new PackageKit::Transaction(this);
    } else {
        m_refresher->reset();
    }
    connect(m_refresher, SIGNAL(changed()), SLOT(populateInstalledCache()), Qt::UniqueConnection);
    //connect(m_refresher, SIGNAL(destroy()), m_refresher, SLOT(deleteLater()));

    m_refresher->refreshCache(false);
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
        if (res->name().contains(searchText, Qt::CaseInsensitive))
            ret += res;
    }
    return ret;
}

int PackageKitBackend::updatesCount() const
{
    int ret = 0;
    for(AbstractResource* res : m_packages.values()) {
        if (res->state() == AbstractResource::Upgradeable && !res->isTechnical()) {
            ret++;
        }
    }
    return ret;
}

int PackageKitBackend::allUpdatesCount() const
{
    int ret = 0;
    for(AbstractResource* res : m_packages.values()) {
        if (res->state() == AbstractResource::Upgradeable) {
            ret++;
        }
    }
    return ret;
}

void PackageKitBackend::removeTransaction(Transaction* t)
{
    qDebug() << "Remove transaction:" << t->resource()->packageName() << "with" << m_transactions.size() << "transactions running";
    int count = m_transactions.removeAll(t);
    Q_ASSERT(count==1);
    TransactionModel::global()->removeTransaction(t);
}

void PackageKitBackend::installApplication(AbstractResource* app, AddonList )
{
    installApplication(app);
}

void PackageKitBackend::installApplication(AbstractResource* app)
{
    PackageKit::Transaction* installTransaction = new PackageKit::Transaction(this);
    installTransaction->installPackage(qobject_cast<PackageKitResource*>(app)->availablePackageId());
    PKTransaction* t = new PKTransaction(app, Transaction::InstallRole, installTransaction);
    m_transactions.append(t);
    TransactionModel::global()->addTransaction(t);
}

void PackageKitBackend::cancelTransaction(AbstractResource* app)
{
    foreach(Transaction* t, m_transactions) {
        PKTransaction* pkt = qobject_cast<PKTransaction*>(t);
        if (pkt->resource() == app) {
            if (pkt->transaction()->allowCancel()) {
                pkt->transaction()->cancel();
                int count = m_transactions.removeAll(t);
                Q_ASSERT(count==1);
                //TransactionModel::global()->cancelTransaction(t);
            } else {
                kWarning() << "trying to cancel a non-cancellable transaction: " << app->name();
            }
            break;
        }
    }
}

void PackageKitBackend::removeApplication(AbstractResource* app)
{
    kDebug() << "Starting to remove" << app->name();
    PackageKit::Transaction* removeTransaction = new PackageKit::Transaction(this);
    removeTransaction->removePackage(qobject_cast<PackageKitResource*>(app)->installedPackageId());
    PKTransaction* t = new PKTransaction(app, Transaction::RemoveRole, removeTransaction);
    m_transactions.append(t);
    TransactionModel::global()->addTransaction(t);
}

QList<AbstractResource*> PackageKitBackend::upgradeablePackages() const
{
    QList<AbstractResource*> ret;
    for(AbstractResource* res : m_packages.values()) {
        if (res->state() == AbstractResource::Upgradeable) {
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


int PackageKitBackend::compare_versions(QString const& a, QString const& b)
{
    /* First split takes pkgrels */
    QStringList withpkgrel1 = a.split("-");
    QStringList withpkgrel2 = b.split("-");
    QString pkgrel1, pkgrel2;

    if (withpkgrel1.size() >= 2) {
        pkgrel1 = withpkgrel1.at(1);
    }
    if (withpkgrel2.size() >= 2) {
        pkgrel2 = withpkgrel2.at(1);
    }

    for (int i = 0; i != withpkgrel1.count(); i++) {
        QString s1( withpkgrel1.at(i) ); /* takes the rest */
        if (withpkgrel2.count() < i)
            return -1;
        QString s2( withpkgrel2.at(i) );

        /* Second split is to separate actual version numbers (or strings) */
        QStringList v1 = s1.split(".");
        QStringList v2 = s2.split(".");

        QStringList::iterator i1 = v1.begin();
        QStringList::iterator i2 = v2.begin();

        for (; i1 < v1.end() && i2 < v2.end() ; i1++, i2++) {
            if ((*i1).length() > (*i2).length())
                return 1;
            if ((*i1).length() < (*i2).length())
                return -1;
            int p1 = i1->toInt();
            int p2 = i2->toInt();

            if (p1 > p2) {
                return 1;
            } else if (p1 < p2) {
                return -1;
            }
        }

        /* This is, like, v1 = 2.3 and v2 = 2.3.1: v2 wins */
        if (i1 == v1.end() && i2 != v2.end()) {
            return -1;
        }

        /* The opposite case as before */
        if (i2 == v2.end() && i1 != v1.end()) {
            return 1;
        }

        /* The rule explained above */
        if ((!pkgrel1.isEmpty() && pkgrel2.isEmpty()) || (pkgrel1.isEmpty() && !pkgrel2.isEmpty())) {
            return 0;
        }

        /* Normal pkgrel comparison */
        int pg1 = pkgrel1.toInt();
        int pg2 = pkgrel2.toInt();

        if (pg1 > pg2) {
            return 1;
        } else if (pg2 > pg1) {
            return -1;
        }
    }

    return 0;
}
