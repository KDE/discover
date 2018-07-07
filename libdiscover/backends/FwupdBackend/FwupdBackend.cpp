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
#include "FwupdReviewsBackend.h"
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
#include <QDebug>
#include <QThread>
#include <QTimer>
#include <QAction>
#include <QMimeDatabase>


DISCOVER_BACKEND_PLUGIN(FwupdBackend)

#define STRING(s) #s // For Project Name and Version
    
FwupdBackend::FwupdBackend(QObject* parent)
    : AbstractResourcesBackend(parent)
    , m_updater(new FwupdUpdater(this))
    , m_reviews(new FwupdReviewsBackend(this))
    , m_fetching(true)
{

    QTimer::singleShot(500, this, &FwupdBackend::toggleFetching);
    connect(m_reviews, &FwupdReviewsBackend::ratingsReady, this, &AbstractResourcesBackend::emitRatingsReady);
    connect(m_updater, &FwupdUpdater::updatesCountChanged, this, &FwupdBackend::updatesCountChanged);

    client = fwupd_client_new ();
    toDownload = g_ptr_array_new_with_free_func (g_free);
    toIgnore = g_ptr_array_new_with_free_func (g_free);

    /* use a custom user agent to provide the fwupd version */
    userAgent = fwupd_build_user_agent (STRING(PROJECT_NAME),STRING(PROJECT_VERSION));
    soupSession = soup_session_new_with_options (SOUP_SESSION_USER_AGENT, userAgent,SOUP_SESSION_TIMEOUT, 10,NULL);
    soup_session_remove_feature_by_type (soupSession,SOUP_TYPE_CONTENT_DECODER);
    
    if (!m_fetching)
           m_reviews->initialize();
    populate(QStringLiteral("Releases"));

    SourcesModel::global()->addSourcesBackend(new FwupdSourcesBackend(this));
}

FwupdBackend::~FwupdBackend()
{
    g_object_unref (client);
    g_ptr_array_unref (toDownload);
    g_ptr_array_unref (toIgnore);
}

gchar* FwupdBackend::FwupdBuildDeviceID(FwupdDevice* device)
{
    g_autofree gchar *tmp = g_strdup (fwupd_device_get_id (device));
    g_strdelimit (tmp, "/", '_');
    return g_strdup_printf ("org.fwupd.%s.device", tmp);
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
    res->setId(QLatin1String(FwupdBuildDeviceID(device)));
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

    if (fwupd_release_get_appstream_id (rel) != NULL)
        res->setId(QLatin1String(fwupd_release_get_appstream_id (rel)));

    /* the same as we have already */
    if (g_strcmp0 (fwupd_device_get_version (device),fwupd_release_get_version (rel)) == 0)
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
    if(fwupd_release_get_vendor(rel) != NULL)
        res->setVendor(QLatin1String(fwupd_release_get_vendor(rel)));
    if(fwupd_release_get_version(rel) != NULL)
        res->setVersion(QLatin1String(fwupd_release_get_version(rel)));
    if(fwupd_release_get_description(rel) != NULL)
        res->setDescription(QLatin1String((fwupd_release_get_description (rel))));
    if(fwupd_release_get_homepage(rel) != NULL)
        res->setHomePage(QUrl(QLatin1String(fwupd_release_get_homepage(rel))));
    if(fwupd_release_get_license(rel))
        res->setLicense(QLatin1String(fwupd_release_get_license(rel)));
    if (fwupd_release_get_uri (rel) != NULL)
    {
        res->m_updateURI = QLatin1String(fwupd_release_get_uri (rel));
    }
}
void FwupdBackend::FwupdSetDeviceDetails(FwupdResource *res,FwupdDevice *dev)
{
    GPtrArray *guids;
    if (fwupd_device_has_flag (dev, FWUPD_DEVICE_FLAG_UPDATABLE))
        res->isLiveUpdatable = true;
    if (fwupd_device_has_flag (dev, FWUPD_DEVICE_FLAG_ONLY_OFFLINE))
        res->isOnlyOffline = true;
    if (fwupd_device_has_flag (dev, FWUPD_DEVICE_FLAG_NEEDS_REBOOT))
        res->needsReboot = true;
    if (!fwupd_device_has_flag (dev, FWUPD_DEVICE_FLAG_INTERNAL))
        res->isDeviceRemoval = true;
    guids = fwupd_device_get_guids (dev);
    if(guids->len > 0)
    {
        g_autofree gchar *guid_str = NULL;
        g_auto(GStrv) tmp = g_new0 (gchar *, guids->len + 1);
        for (int i = 0; i < (int)guids->len; i++)
            tmp[i] = g_strdup ((gchar *)g_ptr_array_index (guids, i));
        guid_str = g_strjoinv (",", tmp);
        res->guidString = guid_str;
    }
    if(fwupd_device_get_name (dev) != NULL)
    {
        g_autofree gchar *vendor_name = NULL;
        if (g_str_has_prefix (fwupd_device_get_name (dev),fwupd_device_get_vendor (dev)))
        {
            vendor_name = g_strdup (fwupd_device_get_name (dev));
        }
        else
        {
            vendor_name = g_strdup_printf ("%s %s",fwupd_device_get_vendor (dev), fwupd_device_get_name (dev));
        }
        res->setName(QLatin1String(vendor_name));
     }
    if(fwupd_device_get_summary (dev) != NULL)
        res->setSummary(QLatin1String(fwupd_device_get_summary(dev)));
    if(fwupd_device_get_vendor(dev) != NULL)
        res->setVendor(QLatin1String(fwupd_device_get_vendor(dev)));
    if(fwupd_device_get_version(dev) != NULL)
        res->setVersion(QLatin1String(fwupd_device_get_version(dev)));
    if(fwupd_device_get_description(dev) != NULL)
        res->setDescription(QLatin1String((fwupd_device_get_description(dev))));
}

void FwupdBackend::populate(const QString& n)
{
    g_autoptr(GPtrArray) devices = NULL;

    /* get devices */
    devices = fwupd_client_get_devices (client, NULL, NULL);

    if (devices != NULL)
    {
        for (int i = 0; i < (int)devices->len; i++)
         {
            FwupdDevice *device = (FwupdDevice *)g_ptr_array_index (devices, i);
            FwupdResource * res = NULL;
            g_autoptr(GPtrArray) releases = NULL;

            /* Devices Which are not updatable */
            // if (!fwupd_device_has_flag (device, FWUPD_DEVICE_FLAG_UPDATABLE))
            //     continue;

            /* add releases */
            res = FwupdCreateDevice(device);
            res->addCategories(n);


            releases = fwupd_client_get_releases (client,res->m_deviceID.toUtf8().constData(),NULL,NULL);

            if (releases != NULL)
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
    g_autoptr(GPtrArray) remotes = NULL;
    g_autoptr(GCancellable) cancellable = g_cancellable_new();
    g_autoptr(GError) error = NULL;
    g_autoptr(GError) error2 = NULL;
    g_autoptr(GPtrArray) devices = NULL;
    g_autoptr(GPtrArray) rels = NULL;

    /* get All devices, Will filter latter */
    devices = fwupd_client_get_devices (client, cancellable, &error);
    if(devices == NULL){
        if (g_error_matches (error,FWUPD_ERROR,FWUPD_ERROR_NOTHING_TO_DO)){
                #ifdef FWUPD_DEBUG
                    qDebug() << "No Devices Found";
                #endif
                FwupdHandleError(&error);
        }
    }
    else{
        for (int i = 0; i < (int)devices->len; i++) {
            FwupdDevice *device = (FwupdDevice *)g_ptr_array_index (devices, i);
            FwupdResource* res;

           res = FwupdCreateDevice(device); //just to test code should be deleted
           m_toUpdate.append(res); //just to test code should be deleted

            if (!fwupd_device_has_flag (device, FWUPD_DEVICE_FLAG_SUPPORTED))
                continue;

            /*Device is Locked Needs Unlocking*/
            if (fwupd_device_has_flag (device, FWUPD_DEVICE_FLAG_LOCKED))
            {
              res = FwupdCreateDevice(device);
              res->setIsDeviceLocked(true);
              m_toUpdate.append(res);
              connect(res, &FwupdResource::stateChanged, this, &FwupdBackend::updatesCountChanged);
              continue;
            }


            rels = fwupd_client_get_upgrades (client,fwupd_device_get_id(device),cancellable, &error2);

            if (rels == NULL) {
                if (g_error_matches (error2,FWUPD_ERROR,FWUPD_ERROR_NOTHING_TO_DO)){
                    qWarning() << "No Packages Found for "<< fwupd_device_get_id(device);
                    FwupdHandleError(&error2);
                    continue;
                }
            }
            else{
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


gchar* FwupdBackend::FwupdGetChecksum(const gchar *filename,GChecksumType checksum_type)
{
    gsize len;
    g_autofree gchar *data = NULL;
    if (!g_file_get_contents (filename, &data, &len, NULL)) {
        qWarning() << "Cannot Access File!" << filename;
        return NULL;
    }
    return g_compute_checksum_for_data (checksum_type, (const guchar *)data, len);
}

bool FwupdBackend::FwupdAddToSchedule(FwupdDevice *device)
{
    FwupdRelease *rel = fwupd_device_get_release_default (device);
    GPtrArray *checksums;
    const gchar *update_uri;
    g_autofree gchar *basename = NULL;
    g_autofree gchar *filename_cache = NULL;
    g_autoptr(GFile) file = NULL;
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
    update_uri = fwupd_release_get_uri (rel);
    if (update_uri == NULL)
    {
        qWarning() << "no location available for" << app->m_name <<  "[" << app->m_id << "]";
        return false;
    }

    /* Checking for firmware in the cache? */
    basename = g_path_get_basename (update_uri);
    filename_cache = FwupdCacheFile("fwupd",basename);
    if (filename_cache == NULL)
        return false;

    if (g_file_test (filename_cache, G_FILE_TEST_EXISTS))
    {
        const gchar *checksum_tmp = NULL;
        g_autofree gchar *checksum = NULL;

        /* Currently LVFS supports SHA1 only*/
        checksum_tmp = fwupd_checksum_get_by_kind (checksums,G_CHECKSUM_SHA1);
        if (checksum_tmp == NULL)
        {
            qWarning() << "No valid checksum for" << filename_cache;
        }
        checksum =  FwupdGetChecksum(filename_cache,G_CHECKSUM_SHA1);
        if (checksum == NULL)
            return false;
        if (g_strcmp0 (checksum_tmp, checksum) != 0)
        {
            qWarning() << filename_cache << " does not match checksum, expected" << checksum_tmp << "got" << checksum;
            g_unlink (filename_cache);
            return false;
        }
    }

    /* link file to application and append to update list */
    file = g_file_new_for_path (filename_cache);
    app->m_file = file;
    m_toUpdate.append(app);
    /* schedule for download */
    if (!g_file_test (filename_cache, G_FILE_TEST_EXISTS))
        FwupdAddToScheduleForDownload(update_uri);

    return true;
}



bool FwupdBackend::FwupdDownloadFile(const gchar *uri,const gchar *filename)
{
    guint status_code;
    g_autoptr(SoupMessage) msg = NULL;
    g_return_val_if_fail (uri != NULL, FALSE);
    g_return_val_if_fail (filename != NULL, FALSE);
    g_autoptr(GCancellable) cancellable = g_cancellable_new();
    g_autoptr(GError) error_local = NULL;
    
    if (g_str_has_prefix (uri, "file://"))
    {
        gsize length = 0;
        g_autofree gchar *contents = NULL;
        if (!g_file_get_contents (uri + 7, &contents, &length, &error_local)) 
        {
            qWarning() << "Cannot Access File" << uri;
            return false;
        }
        if (!g_file_set_contents (filename, contents, length, &error_local))
        {
            qWarning() << "Cannot Save the file content in " << filename;
            return false;
        }
        return true;
    }
    
    /* remote */
    #ifdef FWUPD_DEBUG
        qDebug() << "downloading " << uri << " to " << filename;
    #endif
    msg = soup_message_new (SOUP_METHOD_GET, uri);
    if (msg == NULL)
    {
    //  To DO Error Handling
        return false;
    }

    status_code = soup_session_send_message (soupSession, msg);
    if (status_code != SOUP_STATUS_OK)
    {
        g_autoptr(GString) str = g_string_new (NULL);
        g_string_append (str, soup_status_get_phrase (status_code));
        if (msg->response_body->data != NULL)
        {
            g_string_append (str, ": ");
            g_string_append (str, msg->response_body->data);
        }
    //  To DO Error Handling
        return false;
    }

    if (!g_file_set_contents (filename,msg->response_body->data,msg->response_body->length,&error_local)) 
    {
        qWarning() << "Cannot Save the file content in " << filename;
        return false;
    }
    return true;
}

GBytes* FwupdBackend::FwupdDownloadData(const gchar *uri)
{
    guint status_code;
    g_autoptr(SoupMessage) msg = NULL;
    g_autoptr(GCancellable) cancellable = g_cancellable_new();
    g_autoptr(GError) error = NULL;
    
    g_return_val_if_fail (uri != NULL, NULL);

    /* local */
    if (g_str_has_prefix (uri, "file://")) {
        gsize length = 0;
        g_autofree gchar *contents = NULL;
        g_autoptr(GError) error_local = NULL;

        if (!g_file_get_contents (uri + 7, &contents, &length, &error_local)) {
          //  To DO Error Handling
            return NULL;
        }
        return g_bytes_new (contents, length);
    }

    /* remote */
    msg = soup_message_new (SOUP_METHOD_GET, uri);
    status_code = soup_session_send_message (soupSession, msg);
    if (status_code != SOUP_STATUS_OK) {
        g_autoptr(GString) str = g_string_new (NULL);
        g_string_append (str, soup_status_get_phrase (status_code));
        if (msg->response_body->data != NULL) {
            g_string_append (str, ": ");
            g_string_append (str, msg->response_body->data);
        }
        // TO Do Error Handling
        return NULL;
    }
    return g_bytes_new (msg->response_body->data,(gsize) msg->response_body->length);
}

bool FwupdBackend::FwupdRefreshRemotes(guint cache_age)
{
    g_autoptr(GPtrArray) remotes = NULL;
    g_autoptr(GCancellable) cancellable = g_cancellable_new();
    g_autoptr(GError) error = NULL;

    /*Gets all Remotes will filter later*/
    remotes = fwupd_client_get_remotes (client, cancellable, &error);
    if (remotes == NULL)
        return false;
    for (int i = 0; i < (int)remotes->len; i++) {
        FwupdRemote *remote = (FwupdRemote *)g_ptr_array_index (remotes, i);
        /*Remotes disabled by user so ignore*/
        if (!fwupd_remote_get_enabled (remote))
            continue;
        /*Local Remotes Ignore*/
        if (fwupd_remote_get_kind (remote) == FWUPD_REMOTE_KIND_LOCAL)
            continue;
        /*Refresh the left ones*/
        if (!FwupdRefreshRemote(remote, cache_age))
            return false;
    }
    return true;
}

bool FwupdBackend::FwupdRefreshRemote(FwupdRemote *remote,guint cache_age)
{
    GChecksumType checksum_kind;
    const gchar *url_sig = NULL;
    const gchar *url = NULL;
    g_autoptr(GError) error_local = NULL;
    g_autofree gchar *basename = NULL;
    g_autofree gchar *basename_sig = NULL;
    g_autofree gchar *cache_id = NULL;
    g_autofree gchar *checksum = NULL;
    g_autofree gchar *filename = NULL;
    g_autofree gchar *filename_sig = NULL;
    g_autoptr(GBytes) data = NULL;
    g_autoptr(GCancellable) cancellable = g_cancellable_new();
    g_autoptr(GError) error = NULL;
    
    if (fwupd_remote_get_filename_cache_sig (remote) == NULL) {
        qWarning() << "Remote " << fwupd_remote_get_id (remote) << "has no cache signature!";
        return false;
    }
    
    /* check cache age */
    if (cache_age > 0)
    {
        guint64 age = fwupd_remote_get_age (remote);
        guint tmp = age < G_MAXUINT ? (guint) age : G_MAXUINT;
        if (tmp < cache_age)
        {
            #ifdef FWUPD_DEBUG
                qDebug() << filename_sig << "is only" << tmp << "seconds old, so ignoring refresh! ";
            #endif
            return true;
        }
    }
    
    cache_id = g_strdup_printf ("fwupd/remotes.d/%s", fwupd_remote_get_id (remote));
    basename_sig = g_path_get_basename (fwupd_remote_get_filename_cache_sig (remote)); 
    filename_sig = FwupdCacheFile(cache_id,basename_sig);

    /* download the signature first*/
    url_sig = fwupd_remote_get_metadata_uri_sig (remote);
    #ifdef FWUPD_DEBUG
        qDebug() << "Download Remotes Signature";
    #endif
    data = FwupdDownloadData(url_sig);

    if (data == NULL) {
        //To Do Error Handling
        return false;
    }
    
    checksum_kind = fwupd_checksum_guess_kind (fwupd_remote_get_checksum (remote));
    checksum = g_compute_checksum_for_data (checksum_kind,(const guchar *) g_bytes_get_data (data, NULL),g_bytes_get_size (data));

    if (g_strcmp0 (checksum, fwupd_remote_get_checksum (remote)) == 0)
    {
        #ifdef FWUPD_DEBUG
            qDebug() << "signature of" <<  url_sig << "is unchanged";
        #endif
        return true;
    }

    /* save to a file */
    #ifdef FWUPD_DEBUG
        qDebug() << "saving new remote signature to:" << filename_sig;
    #endif
    if (!g_file_set_contents (filename_sig,(const gchar*)g_bytes_get_data (data, NULL),(guint) g_bytes_get_size (data),&error_local))
    {
        qWarning() << "cannot save signature";
        return false;
    }

    basename = g_path_get_basename (fwupd_remote_get_filename_cache (remote));
    filename = FwupdCacheFile(cache_id,basename);

    if (filename == NULL)
        return FALSE;
    #ifdef FWUPD_DEBUG
        qDebug() << "saving new firmware metadata to:" <<  filename;
        qDebug() << "Downloading remotes metadata";
    #endif
    url = fwupd_remote_get_metadata_uri (remote);
    if (!FwupdDownloadFile (url, filename)) 
    {
        qWarning() << "Cannot Download File : " << filename ;
        return false;
    }

    /* Sending Metadata to fwupd Daemon*/
    if (!fwupd_client_update_metadata (client,fwupd_remote_get_id (remote),filename,filename_sig,cancellable,&error)) 
    {
        FwupdHandleError(&error);
        return false;
    }
    
    return true;
}

void FwupdBackend::FwupdHandleError(GError **perror)
{
    GError *error = perror != NULL ? *perror : NULL;

    if(error == NULL)
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

gchar* FwupdBackend::FwupdCacheFile(const gchar *kind,const gchar *resource)
{
    g_autofree gchar *basename = NULL;
    g_autofree gchar *cachedir = NULL;
    g_autoptr(GFile) cachedir_file = NULL;
    g_autoptr(GPtrArray) candidates = g_ptr_array_new_with_free_func (g_free);
    g_autoptr(GError) error = NULL;
    basename = g_path_get_basename (resource);
    cachedir = g_build_filename (g_get_user_cache_dir (),"discover",kind,NULL);
    cachedir_file = g_file_new_for_path (cachedir);

    if (!g_file_query_exists (cachedir_file, NULL) && !g_file_make_directory_with_parents (cachedir_file, NULL, &error))
            return NULL;
    g_ptr_array_add (candidates, g_build_filename (cachedir, basename, NULL));

    return g_strdup((gchar *)g_ptr_array_index (candidates, 0));
}
bool FwupdBackend::FwupdDownloadAllScheduled(guint cache_age)
{
    const gchar *tmp;
    if (!FwupdRefreshRemotes(cache_age))
        return false;

    /* download the files to the cachedir */
    for (int i = 0; i < (int)toDownload->len; i++)
    {
        g_autoptr(GError) error_local = NULL;
        g_autofree gchar *basename = NULL;
        g_autofree gchar *filename_cache = NULL;

        tmp = (gchar *)g_ptr_array_index (toDownload, i);
        basename = g_path_get_basename (tmp);
        filename_cache = FwupdCacheFile("fwupd", basename);

        if (filename_cache == NULL)
            return false;

        /* download file */
        if (!FwupdDownloadFile(tmp, filename_cache))
        {
            qWarning() <<"Failed to download " << tmp  << ", ignoring:" ;
            g_ptr_array_remove_index (toDownload, i--);
            g_ptr_array_add (toIgnore, g_strdup (tmp));
            continue;
        }
     }

     return true;
}

bool FwupdBackend::FwupdAddToScheduleForDownload(const gchar * uri)
{
    const gchar *tmp;
    for (int i = 0; i < (int)toIgnore->len; i++)
    {
        tmp = (gchar *)g_ptr_array_index (toIgnore, i);
        if (g_strcmp0 (tmp, uri) == 0)
            return false;
    }
    for (int i = 0; i < (int)toDownload->len; i++)
    {
        tmp = (gchar *)g_ptr_array_index (toDownload, i);
        if (g_strcmp0 (tmp, uri) == 0)
            return false;
    }
    g_ptr_array_add (toDownload, g_strdup (uri));
    return true;
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
    //return m_reviews; // To Remove the Review backend ( not needed)
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
    g_autofree gchar *content_type = NULL;
    g_autofree gchar *filename = NULL;
    g_autoptr(GPtrArray) devices = NULL;
    g_autoptr(GCancellable) cancellable = g_cancellable_new();
    g_autoptr(GError) error = NULL;

    QMimeDatabase db;
    QMimeType type = db.mimeTypeForFile(path.fileName());

    if(type.isValid() && type.inherits(QStringLiteral("application/vnd.ms-cab-compressed")))
    {
        filename = path.fileName().toUtf8().data();
        devices = fwupd_client_get_details (client,filename,cancellable,&error);

        if (devices != NULL)
        {
            for (int i = 0; i < (int)devices->len; i++)
            {
                FwupdDevice *dev = (FwupdDevice *)g_ptr_array_index (devices, i);
                FwupdResource* app = NULL;
                app = FwupdCreateRelease(dev);
                app->setState(AbstractResource::None);
                m_resources.insert(app->packageName(), app);
                connect(app, &FwupdResource::stateChanged, this, &FwupdBackend::updatesCountChanged);
                return app;
            }
        }
        else
        {
            FwupdHandleError(&error);
        }
    }
    return NULL;
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
