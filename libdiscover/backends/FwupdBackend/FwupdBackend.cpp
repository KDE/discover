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
#include <resources/StandardBackendUpdater.h>
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
    , m_updater(new StandardBackendUpdater(this))
    , m_fetching(true)
{

    QTimer::singleShot(500, this, &FwupdBackend::toggleFetching);
    connect(m_updater, &StandardBackendUpdater::updatesCountChanged, this, &FwupdBackend::updatesCountChanged);

    client = fwupd_client_new();

    populate();
    SourcesModel::global()->addSourcesBackend(new FwupdSourcesBackend(this));
}

QMap<GChecksumType, QCryptographicHash::Algorithm> FwupdBackend::initHashMap()
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
    g_object_unref(client);
}

QString FwupdBackend::buildDeviceID(FwupdDevice* device)
{
    QString DeviceID = QString::fromUtf8(fwupd_device_get_id(device));
    DeviceID.replace(QLatin1Char('/'),QLatin1Char('_'));
    return QStringLiteral("org.fwupd.%1.device").arg(DeviceID);
}

void FwupdBackend::addResourceToList(FwupdResource* res)
{
    res->setState(FwupdResource::Upgradeable);
    m_resources.insert(res->packageName().toLower(), res);
}

FwupdResource * FwupdBackend::createDevice(FwupdDevice *device)
{
    const QString name = QString::fromUtf8(fwupd_device_get_name(device));
    FwupdResource* res = new FwupdResource(name, true, this);
    res->setId(buildDeviceID(device));
    res->addCategories(QStringLiteral("Releases"));
    if (fwupd_device_get_icons(device)->len >= 1)
        res->setIconName(QString::fromUtf8((const gchar *)g_ptr_array_index(fwupd_device_get_icons(device),0)));// Check wether given icon exists or not!

    setDeviceDetails(res, device);
    return res;
}

FwupdResource * FwupdBackend::createRelease(FwupdDevice *device)
{
    FwupdRelease *release = fwupd_device_get_release_default(device);
    const QString name = QString::fromUtf8(fwupd_release_get_name(release));

    FwupdResource* res = new FwupdResource(name, true, this);

    res->setDeviceID(QString::fromUtf8(fwupd_device_get_id(device)));
    setReleaseDetails(res, release);
    setDeviceDetails(res, device);
    res->setId(QString::fromUtf8(fwupd_release_get_appstream_id(release)));

    /* the same as we have already */
    if (qstrcmp(fwupd_device_get_version(device), fwupd_release_get_version(release)) == 0)
    {
        qWarning() << "Fwupd Error: same firmware version as installed";
    }

    return res;

}
void FwupdBackend::setReleaseDetails(FwupdResource *res, FwupdRelease *release)
{
    res->addCategories(QLatin1String("Releases"));
    if (fwupd_release_get_summary(release))
        res->setSummary(QString::fromUtf8(fwupd_release_get_summary(release)));
    if (fwupd_release_get_vendor(release))
        res->setVendor(QString::fromUtf8(fwupd_release_get_vendor(release)));
    if (fwupd_release_get_size(release))
        res->setSize(fwupd_release_get_size(release));
    if (fwupd_release_get_version(release))
        res->setVersion(QString::fromUtf8(fwupd_release_get_version(release)));
    if (fwupd_release_get_description(release))
        res->setDescription(QString::fromUtf8((fwupd_release_get_description(release))));
    if (fwupd_release_get_homepage(release))
        res->setHomePage(QUrl(QString::fromUtf8(fwupd_release_get_homepage(release))));
    if (fwupd_release_get_license(release))
        res->setLicense(QString::fromUtf8(fwupd_release_get_license(release)));
    if (fwupd_release_get_uri(release))
        res->m_updateURI = QString::fromUtf8(fwupd_release_get_uri(release));
}
void FwupdBackend::setDeviceDetails(FwupdResource *res, FwupdDevice *dev)
{
    res->isLiveUpdatable = fwupd_device_has_flag(dev, FWUPD_DEVICE_FLAG_UPDATABLE);
    res->isOnlyOffline = fwupd_device_has_flag(dev, FWUPD_DEVICE_FLAG_ONLY_OFFLINE);
    res->needsReboot = fwupd_device_has_flag(dev, FWUPD_DEVICE_FLAG_NEEDS_REBOOT);
    res->isDeviceRemoval = !fwupd_device_has_flag(dev, FWUPD_DEVICE_FLAG_INTERNAL);
    res->needsBootLoader = fwupd_device_has_flag(dev, FWUPD_DEVICE_FLAG_NEEDS_BOOTLOADER);

    GPtrArray *guids = fwupd_device_get_guids(dev);
    if (guids->len > 0)
    {
        QString guidStr = QString::fromUtf8((char *)g_ptr_array_index(guids, 0));
        for(uint i = 1; i < guids->len; i++)
        {
            guidStr += QLatin1Char(',') + QString::fromUtf8((char *)g_ptr_array_index(guids, i));
        }
        res->guidString = guidStr;
    }
    if (fwupd_device_get_name(dev))
    {
        QString vendorDesc = QString::fromUtf8(fwupd_device_get_name(dev));
        const QString vendorName = QString::fromUtf8(fwupd_device_get_vendor(dev));

        if (!vendorDesc.startsWith(vendorName))
            vendorDesc = vendorName + QLatin1Char(' ') + vendorDesc;
        res->setName(vendorDesc);
     }
    if (fwupd_device_get_summary(dev))
        res->setSummary(QString::fromUtf8(fwupd_device_get_summary(dev)));
    if (fwupd_device_get_vendor(dev))
        res->setVendor(QString::fromUtf8(fwupd_device_get_vendor(dev)));
    if (fwupd_device_get_created(dev))
        res->setReleaseDate((QDateTime::fromSecsSinceEpoch(fwupd_device_get_created(dev))).date());
    if (fwupd_device_get_version(dev))
        res->setVersion(QString::fromUtf8(fwupd_device_get_version(dev)));
    if (fwupd_device_get_description(dev))
        res->setDescription(QString::fromUtf8((fwupd_device_get_description(dev))));
    res->setIconName(QString::fromUtf8("device-notifier"));
}

void FwupdBackend::populate()
{
    /* get devices */
    g_autoptr(GPtrArray) devices = fwupd_client_get_devices(client, nullptr, nullptr);

    if (devices) {
        for(uint i = 0; i < devices->len; i++) {
            FwupdDevice *device = (FwupdDevice *)g_ptr_array_index(devices, i);

            addResourceToList(createDevice(device));
        }
    }
}


void FwupdBackend::addUpdates()
{
    g_autoptr(GError) error = nullptr;
    g_autoptr(GError) error2 = nullptr;
    g_autoptr(GPtrArray) devices = fwupd_client_get_devices(client, nullptr, &error);

    if (!devices)
    {
        if (g_error_matches(error, FWUPD_ERROR, FWUPD_ERROR_NOTHING_TO_DO))
        {
            qDebug() << "Fwupd Info: No Devices Found";
            handleError(&error);
        }
        return;
    }

    for(uint i = 0; i < devices->len; i++)
    {
        FwupdDevice *device = (FwupdDevice *)g_ptr_array_index(devices, i);

        if (!fwupd_device_has_flag(device, FWUPD_DEVICE_FLAG_SUPPORTED))
            continue;

        /*Device is Locked Needs Unlocking*/
        if (fwupd_device_has_flag(device, FWUPD_DEVICE_FLAG_LOCKED))
        {
            auto res = createDevice(device);
            res->setIsDeviceLocked(true);
            addResourceToList(res);
            connect(res, &FwupdResource::stateChanged, this, &FwupdBackend::updatesCountChanged);
            continue;
        }

        if (!fwupd_device_has_flag(device, FWUPD_DEVICE_FLAG_UPDATABLE))
            continue;


        g_autoptr(GPtrArray) rels = fwupd_client_get_upgrades(client, fwupd_device_get_id(device), nullptr, &error2);
        if (rels) {
            fwupd_device_add_release(device, (FwupdRelease *)g_ptr_array_index(rels, 0));
            auto res = createApp(device);
            if (!res)
            {
                qWarning() << "Fwupd Error: Cannot Create App From Device" << fwupd_device_get_name(device);
            }
            else
            {
                QString longdescription;
                for(uint j = 0; j < rels->len; j++)
                {
                    FwupdRelease *release = (FwupdRelease *)g_ptr_array_index(rels, j);
                    if (!fwupd_release_get_description(release))
                        continue;
                    longdescription += QStringLiteral("Version %1\n").arg(QString::fromUtf8(fwupd_release_get_version(release)));
                    longdescription += QString::fromUtf8(fwupd_release_get_description(release)) + QLatin1Char('\n');
                }
                res->setDescription(longdescription);
                addResourceToList(res);
            }
        } else {
            if (!g_error_matches(error2, FWUPD_ERROR, FWUPD_ERROR_NOTHING_TO_DO))
            {
                handleError(&error2);
            }
        }
    }
}

void FwupdBackend::addHistoricalUpdates()
{
    g_autoptr(GError) error = nullptr;
    g_autoptr(FwupdDevice) device = fwupd_client_get_results(client, FWUPD_DEVICE_ID_ANY, nullptr, &error);
    if (!device)
    {
        handleError(&error);
    }
    else
    {
        FwupdResource* res = createRelease(device);
        if (!res)
            qWarning() << "Fwupd Error: Cannot Make App for" << fwupd_device_get_name(device);
        else
        {
            addResourceToList(res);
        }
    }
}


QByteArray FwupdBackend::getChecksum(const QString &filename, QCryptographicHash::Algorithm hashAlgorithm)
{
    QFile f(filename);
    if (!f.open(QFile::ReadOnly))
        return {};

    QCryptographicHash hash(hashAlgorithm);
    if (!hash.addData(&f))
        return {};

    return hash.result().toHex();
}

FwupdResource* FwupdBackend::createApp(FwupdDevice *device)
{
    FwupdRelease *release = fwupd_device_get_release_default(device);
    GPtrArray *checksums;
    FwupdResource* app = createRelease(device);

    /* update unsupported */
    if (!app->isLiveUpdatable)
    {
        qWarning() << "Fwupd Error: " << app->m_name << "[" << app->m_id << "]" << "cannot be updated ";
        return nullptr;
    }

    /* Important Attributes missing */
    if (app->m_id.isNull())
    {
        qWarning() << "Fwupd Error: No id for firmware";
        return nullptr;
    }
    if (app->m_version.isNull())
    {
        qWarning() << "Fwupd Error: No version! for " << app->m_id;
        return nullptr;
    }
    checksums = fwupd_release_get_checksums(release);
    if (checksums->len == 0)
    {
        qWarning() << "Fwupd Error: " << app->m_name << "[" << app->m_id << "] has no checksums, ignoring as unsafe";
        return nullptr;
    }
    const QUrl update_uri(QString::fromUtf8(fwupd_release_get_uri(release)));

    if (!update_uri.isValid())
    {
        qWarning() << "Fwupd Error: No Update URI available for" << app->m_name <<  "[" << app->m_id << "]";
        return nullptr;
    }

    /* Checking for firmware in the cache? */
    const QString filename_cache = cacheFile(QStringLiteral("fwupd"), QFileInfo(update_uri.path()).fileName());
    Q_ASSERT(!filename_cache.isEmpty());

    const QByteArray checksum_tmp = QByteArray(fwupd_checksum_get_by_kind(checksums, G_CHECKSUM_SHA1));

    /* Currently LVFS supports SHA1 only*/
    if (checksum_tmp.isNull())
    {
        qWarning() << "Fwupd Error: No valid checksum for" << filename_cache;
    }
    QByteArray checksum = getChecksum(filename_cache, QCryptographicHash::Sha1);
    if (checksum.isNull() || checksum_tmp.isNull()) {
        qWarning() << "failed to get checksum from" << filename_cache;
        return nullptr;
    }
    if (checksum_tmp != checksum)
    {
        qWarning() << "Fwupd Error: " << filename_cache << " does not match checksum, expected" << checksum_tmp << "got" << checksum;
        QFile::remove(filename_cache);
        return nullptr;
    }

    /* link file to application and return its reference */
    app->m_file = filename_cache;
    return app;
}

void FwupdBackend::saveFile(QNetworkReply *reply)
{
    QString filename = this->m_downloadFile[reply->url()];
    if (reply->error() == QNetworkReply::NoError)
    {
        QByteArray Data = reply->readAll();
        QFile file(filename);
        if (file.open(QIODevice::WriteOnly))
        {
            file.write(Data);
        }
        else
        {
            qWarning() << "Fwupd Error: Cannot Open File to write Data";
        }
        file.close();
        delete reply;
    }
}

bool FwupdBackend::downloadFile(const QUrl &uri,const QString &filename)
{
    QScopedPointer<QNetworkAccessManager> manager(new QNetworkAccessManager(this));
    this->m_downloadFile.insert(uri,filename);
    QEventLoop loop;
    QTimer getTimer;
    connect(&getTimer, &QTimer::timeout, &loop, &QEventLoop::quit);
    connect(manager.data(), &QNetworkAccessManager::finished, &loop, &QEventLoop::quit);
    QNetworkReply *reply = manager->get(QNetworkRequest(uri));
    getTimer.start(600000); // 60 Seconds TimeOout Period
    loop.exec();
    if (!reply)
    {
        return false;
    }
    else if (QNetworkReply::NoError != reply->error() )
    {
        return false;
    }
    else
    {
        saveFile(reply);
    }
    return true;
}

void FwupdBackend::refreshRemotes(uint cacheAge)
{
    g_autoptr(GError) error = nullptr;
    g_autoptr(GPtrArray) remotes = fwupd_client_get_remotes(client, nullptr, &error);
    if (!remotes)
        return;

    for(uint i = 0; i < remotes->len; i++)
    {
        FwupdRemote *remote = (FwupdRemote *)g_ptr_array_index(remotes, i);
        if (!fwupd_remote_get_enabled(remote))
            continue;

        if (fwupd_remote_get_kind(remote) == FWUPD_REMOTE_KIND_LOCAL)
            continue;

        refreshRemote(remote, cacheAge);
    }
}

void FwupdBackend::refreshRemote(FwupdRemote* remote, uint cacheAge)
{
    if (!fwupd_remote_get_filename_cache_sig(remote))
    {
        qWarning() << "Fwupd Error: " << "Remote " << fwupd_remote_get_id(remote) << "has no cache signature!";
        return;
    }

    /* check cache age */
    if (cacheAge > 0)
    {
        quint64 age = fwupd_remote_get_age(remote);
        uint tmp = age < std::numeric_limits<uint>::max() ? (uint) age : std::numeric_limits<uint>::max();
        if (tmp < cacheAge)
        {
//             qDebug() << "Fwupd Info:" << fwupd_remote_get_id(remote) << "is only" << tmp << "seconds old, so ignoring refresh! ";
            return;
        }
    }

    QString cacheId = QStringLiteral("fwupd/remotes.d/%1").arg(QString::fromUtf8(fwupd_remote_get_id(remote)));
    const auto basenameSig = QString::fromUtf8(g_path_get_basename(fwupd_remote_get_filename_cache_sig(remote)));
    const QString filenameSig = cacheFile(cacheId, basenameSig);

    if (filenameSig.isEmpty())
        return;

    /* download the signature first*/
    const QUrl urlSig(QString::fromUtf8(fwupd_remote_get_metadata_uri_sig(remote)));


    const QString filenameSig_(filenameSig + QStringLiteral(".tmp"));

    if (!downloadFile(urlSig, filenameSig))
    {
        qDebug() << "Fwupd Info: failed to download" << urlSig;
        return;
    }

    QMap<GChecksumType,QCryptographicHash::Algorithm> map = initHashMap();
    QCryptographicHash::Algorithm hashAlgorithm = map[(fwupd_checksum_guess_kind(fwupd_remote_get_checksum(remote)))];
    QByteArray hash = getChecksum(filenameSig_,hashAlgorithm);

    if (fwupd_remote_get_checksum(remote) == hash)
    {
        qDebug() << "Fwupd Info: signature of" << urlSig << "is unchanged";
        return;
    }
    QFile::remove(filenameSig);

    /* save to a file */
    qDebug() << "Fwupd Info: saving new remote signature to:" << filenameSig;

    if (!QFile::rename(filenameSig_, filenameSig)) {
        QFile::remove(filenameSig_);
        qWarning() << "Fwupd Error: cannot save remote signature";
        return;
    }
    QFile::remove(filenameSig_);

    const auto basename = QString::fromUtf8(g_path_get_basename(fwupd_remote_get_filename_cache(remote)));
    const QString filename = cacheFile(cacheId, basename);

    if (filename.isEmpty())
        return;

    qDebug() << "Fwupd Info: saving new firmware metadata to:" <<  filename;

    const QUrl url(QString::fromUtf8(fwupd_remote_get_metadata_uri(remote)));
    if (!downloadFile(url, filename))
    {
        qWarning() << "Fwupd Error: cannot download file : " << filename;
        return;
    }
    /* Sending Metadata to fwupd Daemon*/
    g_autoptr(GError) error = nullptr;
    if (!fwupd_client_update_metadata(client, fwupd_remote_get_id(remote), filename.toUtf8().constData(), filenameSig.toUtf8().constData(), nullptr, &error))
    {
        handleError(&error);
    }
}

void FwupdBackend::handleError(GError **perror)
{
    //TODO: localise the error message
    Q_EMIT passiveMessage(QString::fromUtf8((*perror)->message));
    qWarning() << "Fwupd Error" << (*perror)->code << (*perror)->message;
}

QString FwupdBackend::cacheFile(const QString &kind, const QString &basename)
{
    const QDir cacheDir(QStandardPaths::writableLocation(QStandardPaths::CacheLocation));
    const QString cacheDirFile = cacheDir.filePath(kind);

    if (!QFileInfo::exists(cacheDirFile) && !cacheDir.mkpath(kind))
    {
        qWarning() << "Fwupd Error: cannot make  cache directory!";
        return {};
    }

    return cacheDir.filePath(kind + QLatin1Char('/') + basename);
}

void FwupdBackend::toggleFetching()
{
    m_fetching = !m_fetching;
    refreshRemotes(30*24*60*60); //  Nicer Way to put time? currently 30 days in seconds
    addUpdates();
    addHistoricalUpdates();
    emit fetchingChanged();
}

int FwupdBackend::updatesCount() const
{
    return m_updater->updatesCount();
}

ResultsStream* FwupdBackend::search(const AbstractResourcesBackend::Filters& filter)
{
    if (filter.resourceUrl.scheme() == QLatin1String("fwupd")) {
        return findResourceByPackageName(filter.resourceUrl);
    } else if (!filter.resourceUrl.isEmpty()) {
        return new ResultsStream(QStringLiteral("FwupdStream-void"), {});
    }

    QVector<AbstractResource*> ret;
    foreach(AbstractResource* r, m_resources) {
        if (r->name().contains(filter.search, Qt::CaseInsensitive) || r->comment().contains(filter.search, Qt::CaseInsensitive))
            ret += r;
    }
    return new ResultsStream(QStringLiteral("FwupdStream"), ret);
}

ResultsStream * FwupdBackend::findResourceByPackageName(const QUrl& search)
{
    auto res = search.scheme() == QLatin1String("fwupd") ? m_resources.value(search.host().replace(QLatin1Char('.'), QLatin1Char(' '))) : nullptr;
    if (!res)
    {
        return new ResultsStream(QStringLiteral("FwupdStream"), {});
    }
    else
        return new ResultsStream(QStringLiteral("FwupdStream"), { res });
}

AbstractBackendUpdater* FwupdBackend::backendUpdater() const
{
    return m_updater;
}

AbstractReviewsBackend* FwupdBackend::reviewsBackend() const
{
    return nullptr;
}

Transaction* FwupdBackend::installApplication(AbstractResource* app, const AddonList& addons)
{
    Q_ASSERT(addons.isEmpty());
    return installApplication(app);
}

Transaction* FwupdBackend::installApplication(AbstractResource* app)
{
	return new FwupdTransaction(qobject_cast<FwupdResource*>(app), this);
}

Transaction* FwupdBackend::removeApplication(AbstractResource* /*app*/)
{
    qWarning() << "should not have reached here, it's not possible to uninstall a firmware";
    return nullptr;
}

void FwupdBackend::checkForUpdates()
{
    if (m_fetching)
        return;

    populate();
    toggleFetching();
    QTimer::singleShot(500, this, &FwupdBackend::toggleFetching);
}

AbstractResource * FwupdBackend::resourceForFile(const QUrl& path)
{
    g_autoptr(GError) error = nullptr;

    QMimeDatabase db;
    QMimeType type = db.mimeTypeForFile(path.fileName());
    FwupdResource* app = nullptr;

    if (type.isValid() && type.inherits(QStringLiteral("application/vnd.ms-cab-compressed")))
    {
        g_autofree gchar *filename = path.fileName().toUtf8().data();
        g_autoptr(GPtrArray) devices = fwupd_client_get_details(client, filename, nullptr, &error);

        if (devices)
        {
            FwupdDevice *device = (FwupdDevice *)g_ptr_array_index(devices, 0);
            app = createRelease(device);
            app->setState(AbstractResource::None);
            for(uint i = 1; i < devices->len; i++)
            {
                FwupdDevice *device = (FwupdDevice *)g_ptr_array_index(devices, i);
                FwupdResource* app_ = createRelease(device);
                app_->setState(AbstractResource::None);
            }
            addResourceToList(app);
            connect(app, &FwupdResource::stateChanged, this, &FwupdBackend::updatesCountChanged);
        }
        else
        {
            handleError(&error);
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
    return false;
}

#include "FwupdBackend.moc"
