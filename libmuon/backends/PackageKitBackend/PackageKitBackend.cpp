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
#include <packagekitqt5/Transaction>
#include <packagekitqt5/Daemon>
#include <packagekitqt5/Details>

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
            return i18n("Transaction canceled!");
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
            return i18n("Canceled priority!");
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
    , m_isFetching(false)
{
    bool b = m_appdata.open();
    Q_ASSERT(b && "must be able to open the appstream database");
    reloadPackageList();

    QTimer* t = new QTimer(this);
    connect(t, &QTimer::timeout, this, &PackageKitBackend::updateDatabase);
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

void PackageKitBackend::setFetching(bool f)
{
    if (f != m_isFetching) {
        m_isFetching = f;
        emit fetchingChanged();
    }
}

void PackageKitBackend::reloadPackageList()
{
    setFetching(true);
    m_updatingPackages = m_packages;
    
    if (m_refresher) {
        disconnect(m_refresher, SIGNAL(changed()), this, SLOT(reloadPackageList()));
    }

    PackageKit::Transaction * t = PackageKit::Daemon::global()->getPackages();
    
    connect(t, SIGNAL(finished(PackageKit::Transaction::Exit,uint)), this, SLOT(getPackagesFinished()));
    connect(t, SIGNAL(package(PackageKit::Transaction::Info, QString, QString)), SLOT(addPackage(PackageKit::Transaction::Info, QString, QString)));
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

void PackageKitBackend::getPackagesFinished()
{
    Q_ASSERT(m_isFetching);

    PackageKit::Transaction* transaction = PackageKit::Daemon::global()->getDetails(m_updatingPackages.keys());
    connect(transaction, SIGNAL(details(PackageKit::Details)), SLOT(packageDetails(PackageKit::Details)));
    connect(transaction, SIGNAL(finished(PackageKit::Transaction::Exit,uint)), SLOT(getDetailsFinished(PackageKit::Transaction::Exit,uint)));
}

void PackageKitBackend::getDetailsFinished(PackageKit::Transaction::Exit, uint)
{
//     commented out because it's not the case currently, the finished signal is getting
//     emitted twice, for some reason. *sigh*
//     Q_ASSERT(m_isFetching);
    m_packages = m_updatingPackages;
    setFetching(false);
}

void PackageKitBackend::packageDetails(const PackageKit::Details& details)
{
    Q_ASSERT(m_isFetching);

    PackageKitResource* res = qobject_cast<PackageKitResource*>(m_updatingPackages.value(details.packageId()));
    Q_ASSERT(res);
    res->setDetails(details);
}

void PackageKitBackend::updateDatabase()
{
    qDebug() << "Starting to update the package cache";
    if (!m_refresher) {
        m_refresher = PackageKit::Daemon::global()->refreshCache(false);
        connect(m_refresher, SIGNAL(changed()), SLOT(reloadPackageList()));
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
    kDebug() << "SEARCH FOR" << searchText;
    QList<AbstractResource*> ret;
    for(AbstractResource* res : m_packages.values()) {
        if (res->name().contains(searchText, Qt::CaseInsensitive)) {
            kDebug() << "Got one" << res->name();
            ret += res;
        }
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
                kWarning() << "trying to cancel a non-cancellable transaction: " << app->name();
            }
            break;
        }
    }
}

void PackageKitBackend::removeApplication(AbstractResource* app)
{
    kDebug() << "Starting to remove" << app->name();
    PKTransaction* t = new PKTransaction(app, Transaction::RemoveRole);
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

#include "PackageKitBackend.moc"
