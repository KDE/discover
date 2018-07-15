/***************************************************************************
 *   Copyright © 2013 Aleix Pol Gonzalez <aleixpol@blue-systems.com>       *
 *   Copyright © 2018 Abhijeet Sharma <sharma.abhijeet2096@gmail.com>      *
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

#include "FwupdBackend.h"
#include "FwupdResource.h"
#include "FwupdTransaction.h"
#include "FwupdSourcesBackend.h"
#include <FwupdUpdater.h>
#include <resources/SourcesModel.h>
#include <Transaction/Transaction.h>

#include <KAboutData>
#include <KLocalizedString>
#include <KPluginFactory>
#include <KConfigGroup>
#include <KSharedConfig>

DISCOVER_BACKEND_PLUGIN(FwupdBackend)
    
FwupdBackend::FwupdBackend(QObject* parent)
    : AbstractResourcesBackend(parent)
    , m_updater(new FwupdUpdater(this))
    , m_fetching(true)
{

    QTimer::singleShot(500, this, &FwupdBackend::toggleFetching);
    connect(m_updater, &FwupdUpdater::updatesCountChanged, this, &FwupdBackend::updatesCountChanged);

    client = fwupd_client_new ();

    populate(QStringLiteral("Releases"));
    SourcesModel::global()->addSourcesBackend(new FwupdSourcesBackend(this));
}

QMap<GChecksumType,QCryptographicHash::Algorithm> FwupdBackend::initHashMap()
{
    QMap<GChecksumType,QCryptographicHash::Algorithm> map;
    
    map.insert(G_CHECKSUM_SHA1,QCryptographicHash::Sha1);
    map.insert(G_CHECKSUM_SHA256,QCryptographicHash::Sha256);
    map.insert(G_CHECKSUM_SHA512,QCryptographicHash::Sha512);
    map.insert(G_CHECKSUM_MD5,QCryptographicHash::Md5);
    
    return map;
}

FwupdBackend::~FwupdBackend()
{
    g_object_unref (client);
    toDownload.clear();
    toIgnore.clear();
}

QString FwupdBackend::FwupdBuildDeviceID(FwupdDevice* device)
{
    QString DeviceID = QLatin1String(fwupd_device_get_id (device));
    DeviceID.replace(QLatin1Char('/'),QLatin1Char('_'));
    return QStringLiteral("org.fwupd.%1.device").arg(DeviceID);
}

QString FwupdBackend::FwupdGetAppName(QString ID)
{
    //To Do Implement it!
    return ID;
}

QSet<AbstractResource*> FwupdBackend::FwupdGetAllUpdates()
{
    QSet<AbstractResource*> ret;
    ret.reserve(m_toUpdate.size());
    foreach(FwupdResource* r, m_toUpdate)
    {
        AbstractResource* res = (AbstractResource*) r;
        if(r->m_id.isEmpty())
            qDebug() << "Resource ID is Empty" << r->m_name;
        ret.insert(res);
    }
    return ret;

}

FwupdResource * FwupdBackend::FwupdCreateDevice(FwupdDevice *device)
{
    const QString name = QLatin1String(fwupd_device_get_name(device));
    FwupdResource* res = new FwupdResource(name, true, this);
    res->setId(FwupdBuildDeviceID(device));
    res->addCategories(QStringLiteral("Releases"));
    res->setIconName(QLatin1String((const gchar *)g_ptr_array_index (fwupd_device_get_icons(device),0)));// Implement a Better way to decide icon

    FwupdSetDeviceDetails(res,device);
    return res;
}

FwupdResource * FwupdBackend::FwupdCreateRelease(FwupdDevice *device)
{
    FwupdRelease *rel = fwupd_device_get_release_default (device);
    const QString name = QLatin1String(fwupd_release_get_name(rel));
    FwupdResource* res = new FwupdResource(name, true, this);

    res->setDeviceID(QLatin1String(fwupd_device_get_id (device)));
    FwupdSetReleaseDetails(res,rel);
    FwupdSetDeviceDetails(res,device);

    if (fwupd_release_get_appstream_id (rel))
        res->setId(QLatin1String(fwupd_release_get_appstream_id (rel)));

    /* the same as we have already */
    if(QLatin1Literal(fwupd_device_get_version (device)) == QLatin1Literal(fwupd_release_get_version (rel)))
    {
        qWarning() << "same firmware version as installed";
    }

    return res;

}
void FwupdBackend::FwupdSetReleaseDetails(FwupdResource *res,FwupdRelease *rel)
{
    res->addCategories(QLatin1String("Releases"));
    if(fwupd_release_get_summary(rel))
        res->setSummary(QLatin1String(fwupd_release_get_summary(rel)));
    if(fwupd_release_get_vendor(rel))
        res->setVendor(QLatin1String(fwupd_release_get_vendor(rel)));
    if(fwupd_release_get_version(rel))
        res->setVersion(QLatin1String(fwupd_release_get_version(rel)));
    if(fwupd_release_get_description(rel))
        res->setDescription(QLatin1String((fwupd_release_get_description (rel))));
    if(fwupd_release_get_homepage(rel))
        res->setHomePage(QUrl(QLatin1String(fwupd_release_get_homepage(rel))));
    if(fwupd_release_get_license(rel))
        res->setLicense(QLatin1String(fwupd_release_get_license(rel)));
    if (fwupd_release_get_uri (rel))
        res->m_updateURI = QLatin1String(fwupd_release_get_uri (rel));
}
void FwupdBackend::FwupdSetDeviceDetails(FwupdResource *res,FwupdDevice *dev)
{
    res->isLiveUpdatable = fwupd_device_has_flag (dev, FWUPD_DEVICE_FLAG_UPDATABLE);
    res->isOnlyOffline = fwupd_device_has_flag (dev, FWUPD_DEVICE_FLAG_ONLY_OFFLINE);
    res->needsReboot = fwupd_device_has_flag (dev, FWUPD_DEVICE_FLAG_NEEDS_REBOOT);
    res->isDeviceRemoval = !fwupd_device_has_flag (dev, FWUPD_DEVICE_FLAG_INTERNAL);
    
    GPtrArray *guids = fwupd_device_get_guids (dev);
    if(guids->len > 0)
    {
        QString guidStr = QLatin1Literal((char *)g_ptr_array_index (guids, 0));
        for (uint i = 1; i < guids->len; i++)
        {
            guidStr += QLatin1Char(',') + QLatin1Literal((char *)g_ptr_array_index (guids, i));
        }
        res->guidString = guidStr;
    }
    if(fwupd_device_get_name (dev))
    {
        QString vendorName;
        vendorName.sprintf("%s",fwupd_device_get_name (dev));
        
        if(vendorName.indexOf(QLatin1String(fwupd_device_get_vendor (dev))) == 0)
            vendorName.sprintf("%s %s",fwupd_device_get_vendor (dev), fwupd_device_get_name (dev));
        res->setName(vendorName);
     }
    if(fwupd_device_get_summary (dev))
        res->setSummary(QLatin1String(fwupd_device_get_summary(dev)));
    if(fwupd_device_get_vendor(dev))
        res->setVendor(QLatin1String(fwupd_device_get_vendor(dev)));
    if(fwupd_device_get_version(dev))
        res->setVersion(QLatin1String(fwupd_device_get_version(dev)));
    if(fwupd_device_get_description(dev))
        res->setDescription(QLatin1String((fwupd_device_get_description(dev))));
}

void FwupdBackend::populate(const QString& n)
{
    /* get devices */
    g_autoptr(GPtrArray) devices = fwupd_client_get_devices (client, NULL, NULL);

    if (devices)
    {
        for (uint i = 0; i < devices->len; i++)
         {
            FwupdDevice *device = (FwupdDevice *)g_ptr_array_index (devices, i);

            /* Devices Which are not updatable */
            if (!fwupd_device_has_flag (device, FWUPD_DEVICE_FLAG_UPDATABLE))
                continue;

            /* add releases */
            FwupdResource * res = FwupdCreateDevice(device);
            res->addCategories(n);


            g_autoptr(GPtrArray) releases = fwupd_client_get_releases (client,res->m_deviceID.toUtf8().constData(),NULL,NULL);

            if (releases)
            {
                for (int j = 0; j < (int)releases->len; j++)
                {
                    FwupdRelease *rel = (FwupdRelease *)g_ptr_array_index (releases, j);
                    const QString name = QLatin1String(fwupd_release_get_name(rel));
                    FwupdResource* res_ = new FwupdResource(name, true, this);
                    FwupdSetReleaseDetails (res_, rel);
                    res->m_releases.append(res_);
                }
            }
            /* add all Valid Resources */
            m_resources.insert(res->name().toLower(), res);
          }
    }
}


void FwupdBackend::FwupdAddUpdates()
{
    g_autoptr(GCancellable) cancellable = g_cancellable_new();
    g_autoptr(GError) error = NULL;
    g_autoptr(GError) error2 = NULL;
    g_autoptr(GPtrArray) devices = fwupd_client_get_devices (client, cancellable, &error);
    g_autoptr(GPtrArray) rels = NULL;
    
    if(!devices)
    {
        if (g_error_matches (error,FWUPD_ERROR,FWUPD_ERROR_NOTHING_TO_DO))
        {
                #ifdef FWUPD_DEBUG
                    qDebug() << "No Devices Found";
                #endif
                FwupdHandleError(&error);
        }
    }
    else{
        for (uint i = 0; i < devices->len; i++)
        {
            FwupdDevice *device = (FwupdDevice *)g_ptr_array_index (devices, i);
            FwupdResource* res;

            res = FwupdCreateDevice(device); //just to test code should be deleted
            if(!m_toUpdate.contains(res)) // Just To Test code Should Be deleted 
                m_toUpdate.append(res); //just to test code should be deleted

            if (!fwupd_device_has_flag (device, FWUPD_DEVICE_FLAG_SUPPORTED))
                continue;

            /*Device is Locked Needs Unlocking*/
            if (fwupd_device_has_flag (device, FWUPD_DEVICE_FLAG_LOCKED))
            {
                res = FwupdCreateDevice(device);
                res->setIsDeviceLocked(true);
                if(!m_toUpdate.contains(res))
                    m_toUpdate.append(res);
                connect(res, &FwupdResource::stateChanged, this, &FwupdBackend::updatesCountChanged);
                continue;
            }


            rels = fwupd_client_get_upgrades (client,fwupd_device_get_id(device),cancellable, &error2);

            if (!rels) 
            {
                if (g_error_matches (error2,FWUPD_ERROR,FWUPD_ERROR_NOTHING_TO_DO))
                {
                    qWarning() << "No Packages Found for "<< fwupd_device_get_id(device);
                    FwupdHandleError(&error2);
                    continue;
                }
            }
            else
            {
                fwupd_device_add_release(device,(FwupdRelease *)g_ptr_array_index(rels,0));
                if(FwupdAddToSchedule(device))
                {
                    qWarning() << "Cannot Add To Schdule" << fwupd_device_get_id(device);
                    continue;
                }
            }
        }
    }
}


QByteArray FwupdBackend::FwupdGetChecksum(const QUrl filename,QCryptographicHash::Algorithm hashAlgorithm)
{
    QFile f(filename.toString());
    if (f.open(QFile::ReadOnly))
    {
        QCryptographicHash hash(hashAlgorithm);
        if (hash.addData(&f)) 
        {
            return hash.result().toHex();
        }
    }
    return QByteArray();
}

bool FwupdBackend::FwupdAddToSchedule(FwupdDevice *device)
{
    FwupdRelease *rel = fwupd_device_get_release_default (device);
    GPtrArray *checksums;
    FwupdResource* app = NULL;

    /* update unsupported */
    app = FwupdCreateRelease(device);
    if (!app->isLiveUpdatable)
    {
        qWarning()  << app->m_name << "[" << app->m_id << "]" << "cannot be updated ";
        return false;
    }

    /* Important Attributes missing */
    if (app->m_id.isNull())
    {
        qWarning() << "fwupd: No id for firmware";
        return true;
    }
    if (app->m_version.isNull())
    {
        qWarning() << "fwupd: No version! for " << app->m_id;
        return true;
    }
    if (app->m_updateVersion.isNull())
    {
        qWarning() << "fwupd: No update-version! for " << app->m_id;
        return true;
    }
    checksums = fwupd_release_get_checksums (rel);
    if (checksums->len == 0)
    {
        qWarning() << app->m_name << "[" << app->m_id << "]" << "(" << app->m_updateVersion << ")" "has no checksums, ignoring as unsafe";
        return false;
    }
    const QUrl update_uri(QLatin1String(fwupd_release_get_uri(rel)));
    
    if (!update_uri.isValid())
    {
        qWarning() << "no location available for" << app->m_name <<  "[" << app->m_id << "]";
        return false;
    }

    /* Checking for firmware in the cache? */
    QFileInfo basename = QFileInfo(update_uri.path());
    const QUrl filename_cache = FwupdCacheFile(QStringLiteral("fwupd"),basename);
    if (!filename_cache.isValid())
        return false;

    if (filename_cache.isLocalFile())
    {
        QByteArray checksum_tmp = QByteArray(fwupd_checksum_get_by_kind (checksums,G_CHECKSUM_SHA1));

        /* Currently LVFS supports SHA1 only*/
        if (checksum_tmp.isNull())
        {
            qWarning() << "No valid checksum for" << filename_cache;
        }
        QByteArray checksum =  FwupdGetChecksum(filename_cache,QCryptographicHash::Sha1);
        if (checksum.isNull())
            return false;
        if (checksum_tmp != checksum)
        {
            qWarning() << filename_cache << " does not match checksum, expected" << checksum_tmp << "got" << checksum;
            QFile::remove((filename_cache.toString()));
            return false;
        }
    }

    /* link file to application and append to update list */
    QFile file(filename_cache.toString());
    app->m_file = &file;
    if(!m_toUpdate.contains(app))
        m_toUpdate.append(app);
    /* schedule for download */
    if (!filename_cache.isLocalFile())
        FwupdAddToScheduleForDownload(update_uri);

    return true;
}

bool FwupdBackend::FwupdDownloadFile(const QUrl &uri,const QString &filename)
{
    QEventLoop loop;
    QTimer getTimer;
    QString fileName = filename;
    QNetworkAccessManager *manager = new QNetworkAccessManager(this);
    QTimer::connect(&getTimer,SIGNAL(timeout()),&loop, SLOT(quit()));
    connect(manager, SIGNAL(finished(QNetworkReply*)),&loop, SLOT(quit()));
    QNetworkReply *reply = manager->get(QNetworkRequest(uri));
    getTimer.start(600000); // 60 Seconds TimeOout Period
    loop.exec();
    if(!reply)
    {
        return false;
    }
    else if( QNetworkReply::NoError != reply->error() )
    {
        return false;
    }
    else
    {
        QByteArray Data = reply->readAll();
        QFile file(fileName);
        if (file.open(QIODevice::WriteOnly)) 
        {
            file.write(Data);
        }
        file.close();
        delete reply;
        delete manager;
        return true;
    }
}

bool FwupdBackend::FwupdRefreshRemotes(uint cacheAge)
{
    g_autoptr(GCancellable) cancellable = g_cancellable_new();
    g_autoptr(GError) error = NULL;
    g_autoptr(GPtrArray)  remotes = fwupd_client_get_remotes (client, cancellable, &error);
    if (!remotes)
        return false;
    for (uint i = 0; i < remotes->len; i++)
    {
        FwupdRemote *remote = (FwupdRemote *)g_ptr_array_index (remotes, i);
        /*Remotes disabled by user so ignore*/
        if (!fwupd_remote_get_enabled (remote))
            continue;
        /*Local Remotes Ignore*/
        if (fwupd_remote_get_kind (remote) == FWUPD_REMOTE_KIND_LOCAL)
            continue;
        /*Refresh the left ones*/
        if (!FwupdRefreshRemote(remote, cacheAge))
            return false;
    }
    return true;
}

bool FwupdBackend::FwupdRefreshRemote(FwupdRemote* remote,uint cacheAge)
{
    g_autoptr(GCancellable) cancellable = g_cancellable_new();
    g_autoptr(GError) error = NULL;
    
    if (fwupd_remote_get_filename_cache_sig (remote) == NULL) 
    {
        qWarning() << "Remote " << fwupd_remote_get_id (remote) << "has no cache signature!";
        return false;
    }
    
    /* check cache age */
    if (cacheAge > 0)
    {
        quint64 age = fwupd_remote_get_age (remote);
        uint tmp = age < std::numeric_limits<uint>::max() ? (uint) age : std::numeric_limits<uint>::max();
        if (tmp < cacheAge)
        {
            #ifdef FWUPD_DEBUG
                qDebug() << remote << "is only" << tmp << "seconds old, so ignoring refresh! ";
            #endif
            return true;
        }
    }
    
    QString cacheId = QStringLiteral("fwupd/remotes.d/%1").arg(QString::fromUtf8(fwupd_remote_get_id (remote))); 
    QFileInfo basenameSig = QFileInfo(QString::fromUtf8(g_path_get_basename(fwupd_remote_get_filename_cache_sig (remote))));
    const QUrl filename_sig = FwupdCacheFile(cacheId,basenameSig);
    
    if (filename_sig.isEmpty())
        return false;
    
    /* download the signature first*/
    const QUrl url_sig(QLatin1String(fwupd_remote_get_metadata_uri_sig(remote)));
    #ifdef FWUPD_DEBUG
        qDebug() << "downloading remotes signatures ...";
    #endif
    
    const QUrl filename_sig_(filename_sig.toString() + QStringLiteral(".tmp"));

    if(!FwupdDownloadFile(url_sig,filename_sig_.toString()))
    {
        qDebug() << "remote signature download failed ...";
        return false;
    }
    
    QMap<GChecksumType,QCryptographicHash::Algorithm> map = initHashMap();
    QCryptographicHash::Algorithm hashAlgorithm = map[(fwupd_checksum_guess_kind (fwupd_remote_get_checksum (remote)))];
    QByteArray hash = FwupdGetChecksum(filename_sig_,hashAlgorithm);
    
    if (fwupd_remote_get_checksum (remote) == hash)
    {
        #ifdef FWUPD_DEBUG
            qDebug() << "signature of" <<  url_sig.toString() << "is unchanged";
        #endif
        return true;
    }
    
    /* save to a file */
    #ifdef FWUPD_DEBUG
        qDebug() << "saving new remote signature to:" << filename_sig.toString();
    #endif

    if (!(QFile::copy(filename_sig_.toString(),filename_sig.toString())))
    {
        qWarning() << "cannot save remote signature";
        return false;
    }
    else
    {
        QFile::remove(filename_sig_.toString());
    }

    QFileInfo basename = QFileInfo(QString::fromUtf8(g_path_get_basename (fwupd_remote_get_filename_cache (remote))));
    const QUrl filename = FwupdCacheFile(cacheId,basename);
    
    if (filename.isEmpty())
        return false;
    
    #ifdef FWUPD_DEBUG
        qDebug() << "saving new firmware metadata to:" <<  filename;
    #endif
    
    const QUrl url(QLatin1String(fwupd_remote_get_metadata_uri (remote)));
    if (!FwupdDownloadFile (url, filename.toString())) 
    {
        qWarning() << "cannot download file : " << filename ;
        return false;
    }
    /* Sending Metadata to fwupd Daemon*/
    if (!fwupd_client_update_metadata (client,fwupd_remote_get_id (remote),filename.toString().toUtf8().constData(),filename_sig.toString().toUtf8().constData(),cancellable,&error)) 
    {
        FwupdHandleError(&error);
        return false;
    }
    return true;
}

void FwupdBackend::FwupdHandleError(GError **perror)
{
    GError *error = perror != NULL ? *perror : NULL;

    if(!error)
        return;
    //To DO Indivitual take action based on case,Show Notification on Discover;
    switch (error->code)
    {
        case FWUPD_ERROR_ALREADY_PENDING:
            qWarning() << "FWUPD_ERROR_ALREADY_PENDING";
            break;
        case FWUPD_ERROR_INVALID_FILE:
            qWarning() << "FWUPD_ERROR_INVALID_FILE";
            break;
        case FWUPD_ERROR_NOT_SUPPORTED:
            qWarning() << "FWUPD_ERROR_NOT_SUPPORTED";
            break;
        case FWUPD_ERROR_AUTH_FAILED:
            qWarning() << "FWUPD_ERROR_AUTH_FAILED";
            break;
        case FWUPD_ERROR_SIGNATURE_INVALID:
            qWarning() << "FWUPD_ERROR_SIGNATURE_INVALID";
            break;
        case FWUPD_ERROR_AC_POWER_REQUIRED:
            qWarning() << "FWUPD_ERROR_AC_POWER_REQUIRED";
           break;
        default:
            qWarning() << "Unknown Error ::" << error->code;
            break;
    }
}

const QUrl FwupdBackend::FwupdCacheFile(const QString &kind,const QFileInfo &resource)
{
    QString basename = resource.fileName();
    const QDir cacheDir(QStandardPaths::writableLocation(QStandardPaths::CacheLocation));
    const QUrl cacheDirFile = QUrl::fromLocalFile(QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + QLatin1Char('/') + kind);
    const QUrl fileUrl = QUrl::fromLocalFile(QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + QLatin1Char('/') + kind + QLatin1Char('/') + basename);
    
    if(!QFileInfo::exists(cacheDirFile.toLocalFile()) && !cacheDir.mkpath(kind)) 
    {
        #ifdef FWUPD_DEBUG
            qDebug() << "cannot make  cache directory!";
        #endif
        return QUrl();
    }
    return QUrl(fileUrl.toString().remove(QStringLiteral("file://")));
}
bool FwupdBackend::FwupdDownloadAllScheduled(uint cacheAge)
{
    if (!FwupdRefreshRemotes(cacheAge))
        return false;

    /* download the files to the cachedir */
    foreach(QUrl uri, toDownload)
    {
        const QUrl filename_cache = FwupdCacheFile(QStringLiteral("fwupd"),QFileInfo(uri.fileName()));
        if(!filename_cache.isValid())
            return false;
        /* download file */
        if(!FwupdDownloadFile(uri,filename_cache.toString()))
        {
            qWarning() <<"Failed to download " << uri.path() << "so, ignoring:" ;
            if(!toIgnore.contains(uri))
                toIgnore.append(uri);
        }
    }

     return true;
}

bool FwupdBackend::FwupdAddToScheduleForDownload(const QUrl uri)
{
    if(toIgnore.contains(uri))
        return false;
    if(toDownload.contains(uri))
        return false;
    else
    {
        toDownload.append(uri);
        return true;
    }
}


void FwupdBackend::toggleFetching()
{
    m_fetching = !m_fetching;
    #ifdef FWUPD_DEBUG
        qDebug() << "Fwupd fetching..." << m_fetching;
    #endif
    FwupdAddUpdates();
    FwupdDownloadAllScheduled(60*60*24*30); //  Nicer Way to put time? currently 30 days in seconds 
    emit fetchingChanged();
}

int FwupdBackend::updatesCount() const
{
    return m_updater->updatesCount();
}

ResultsStream* FwupdBackend::search(const AbstractResourcesBackend::Filters& filter)
{
    QVector<AbstractResource*> ret;
    if (!filter.resourceUrl.isEmpty() && filter.resourceUrl.scheme() == QLatin1String("fwupd"))
        return findResourceByPackageName(filter.resourceUrl);
    else foreach(AbstractResource* r, m_resources) {
        if(r->name().contains(filter.search, Qt::CaseInsensitive) || r->comment().contains(filter.search, Qt::CaseInsensitive))
            ret += r;
    }
    return new ResultsStream(QStringLiteral("Firmware Updates Stream"), ret);
}

ResultsStream * FwupdBackend::findResourceByPackageName(const QUrl& search)
{
    auto res = search.scheme() == QLatin1String("fwupd") ? m_resources.value(search.host().replace(QLatin1Char('.'), QLatin1Char(' '))) : NULL;
    if (!res) {
        return new ResultsStream(QStringLiteral("Firmware Updates Stream"), {});
    } else
        return new ResultsStream(QStringLiteral("Firmware Updates Stream"), { res });
}

AbstractBackendUpdater* FwupdBackend::backendUpdater() const
{
    return m_updater;
}

AbstractReviewsBackend* FwupdBackend::reviewsBackend() const
{
    return NULL;
}

Transaction* FwupdBackend::installApplication(AbstractResource* app, const AddonList& addons)
{
    return new FwupdTransaction(qobject_cast<FwupdResource*>(app), this, addons, Transaction::InstallRole);
}

Transaction* FwupdBackend::installApplication(AbstractResource* app)
{
	return new FwupdTransaction(qobject_cast<FwupdResource*>(app), this, Transaction::InstallRole);
}

Transaction* FwupdBackend::removeApplication(AbstractResource* app)
{
	return new FwupdTransaction(qobject_cast<FwupdResource*>(app), this, Transaction::RemoveRole);
}

void FwupdBackend::checkForUpdates()
{
    if(m_fetching)
        return;
    toggleFetching();
    populate(QStringLiteral("Releases"));
    QTimer::singleShot(500, this, &FwupdBackend::toggleFetching);
    #ifdef FWUPD_DEBUG
        qDebug() << "FwupdBackend::checkForUpdates";
    #endif
}

AbstractResource * FwupdBackend::resourceForFile(const QUrl& path)
{
    g_autoptr(GCancellable) cancellable = g_cancellable_new();
    g_autoptr(GError) error = NULL;

    QMimeDatabase db;
    QMimeType type = db.mimeTypeForFile(path.fileName());
    FwupdResource* app = NULL;
    
    if(type.isValid() && type.inherits(QStringLiteral("application/vnd.ms-cab-compressed")))
    {
        g_autofree gchar *filename = path.fileName().toUtf8().data();
        g_autoptr(GPtrArray) devices = fwupd_client_get_details (client,filename,cancellable,&error);

        if (devices)
        {
            FwupdDevice *dev = (FwupdDevice *)g_ptr_array_index (devices, 0);
            app = FwupdCreateRelease(dev);
            app->setState(AbstractResource::None);
            for (uint i = 1; i < devices->len; i++)
            {
                FwupdDevice *dev = (FwupdDevice *)g_ptr_array_index (devices, i);
                FwupdResource* app_ = FwupdCreateRelease(dev);
                app_->setState(AbstractResource::None);
                if(!app->m_releases.contains(app_))
                    app->m_releases.append(app_);
            }
            m_resources.insert(app->packageName(), app);
            if(!m_toUpdate.contains(app))    
                m_toUpdate.append(app);
            connect(app, &FwupdResource::stateChanged, this, &FwupdBackend::updatesCountChanged);
        }
        else
        {
            FwupdHandleError(&error);
        }
    }
    return app;
}

QString FwupdBackend::displayName() const
{
    return QStringLiteral("Firmware Updates");
}

bool FwupdBackend::hasApplications() const
{
    return m_resources.count() ? true : false;
}

#include "FwupdBackend.moc"
