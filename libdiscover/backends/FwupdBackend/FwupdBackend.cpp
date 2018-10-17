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

#include <QtConcurrent>
#include <KAboutData>
#include <KLocalizedString>
#include <KPluginFactory>
#include <KConfigGroup>
#include <KSharedConfig>

DISCOVER_BACKEND_PLUGIN(FwupdBackend)

FwupdBackend::FwupdBackend(QObject* parent)
    : AbstractResourcesBackend(parent)
    , client(fwupd_client_new())
    , m_updater(new StandardBackendUpdater(this))
{
    connect(m_updater, &StandardBackendUpdater::updatesCountChanged, this, &FwupdBackend::updatesCountChanged);

    SourcesModel::global()->addSourcesBackend(new FwupdSourcesBackend(this));
    QTimer::singleShot(0, this, &FwupdBackend::checkForUpdates);
}

QMap<GChecksumType, QCryptographicHash::Algorithm> FwupdBackend::gchecksumToQChryptographicHash()
{
    static QMap<GChecksumType,QCryptographicHash::Algorithm> map;
    if (map.isEmpty()) {
        map.insert(G_CHECKSUM_SHA1,QCryptographicHash::Sha1);
        map.insert(G_CHECKSUM_SHA256,QCryptographicHash::Sha256);
        map.insert(G_CHECKSUM_SHA512,QCryptographicHash::Sha512);
        map.insert(G_CHECKSUM_MD5,QCryptographicHash::Md5);
    }
    return map;
}

FwupdBackend::~FwupdBackend()
{
    g_object_unref(client);
}

void FwupdBackend::addResourceToList(FwupdResource* res)
{
    res->setParent(this);
    auto &r = m_resources[res->packageName().toLower()];
    if (r) {
        Q_EMIT resourceRemoved(r);
        delete r;
    }
    r = res;
    Q_ASSERT(m_resources.value(res->packageName().toLower()) == res);
}

FwupdResource * FwupdBackend::createDevice(FwupdDevice *device)
{
    const QString name = QString::fromUtf8(fwupd_device_get_name(device));
    FwupdResource* res = new FwupdResource(name, nullptr);

    const QString deviceID = QString::fromUtf8(fwupd_device_get_id(device));
    res->setId(QStringLiteral("org.fwupd.%1.device").arg(QString(deviceID).replace(QLatin1Char('/'),QLatin1Char('_'))));
    res->setDeviceId(deviceID);

    res->setDeviceDetails(device);
    return res;
}

FwupdResource * FwupdBackend::createRelease(FwupdDevice *device)
{
    FwupdResource* res = createDevice(device);

    FwupdRelease *release = fwupd_device_get_release_default(device);
    res->setId(QString::fromUtf8(fwupd_release_get_appstream_id(release)));
    res->setReleaseDetails(release);

    /* the same as we have already */
    if (qstrcmp(fwupd_device_get_version(device), fwupd_release_get_version(release)) == 0)
    {
        qWarning() << "Fwupd Error: same firmware version as installed";
    }

    return res;

}

void FwupdBackend::addUpdates()
{
    g_autoptr(GError) error = nullptr;
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

        g_autoptr(GError) error2 = nullptr;
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
    FwupdResource* app = createRelease(device);

    if (!app->isLiveUpdatable()) {
        qWarning() << "Fwupd Error: " << app->name() << "[" << app->id() << "]" << "cannot be updated ";
        return nullptr;
    }

    if (app->id().isNull()) {
        qWarning() << "Fwupd Error: No id for firmware";
        return nullptr;
    }

    if (app->availableVersion().isNull()) {
        qWarning() << "Fwupd Error: No version! for " << app->id();
        return nullptr;
    }

    GPtrArray *checksums = fwupd_release_get_checksums(release);
    if (checksums->len == 0) {
        qWarning() << "Fwupd Error: " << app->name() << "[" << app->id() << "] has no checksums, ignoring as unsafe";
        return nullptr;
    }

    const QUrl update_uri(QString::fromUtf8(fwupd_release_get_uri(release)));
    if (!update_uri.isValid()) {
        qWarning() << "Fwupd Error: No Update URI available for" << app->name() <<  "[" << app->id() << "]";
        return nullptr;
    }

    /* Checking for firmware in the cache? */
    const QString filename_cache = cacheFile(QStringLiteral("fwupd"), QFileInfo(update_uri.path()).fileName());
    Q_ASSERT(!filename_cache.isEmpty());

    /* Currently LVFS supports SHA1 only*/
    const QByteArray checksum_tmp(fwupd_checksum_get_by_kind(checksums, G_CHECKSUM_SHA1));
    const QByteArray checksum = getChecksum(filename_cache, QCryptographicHash::Sha1);
    if (checksum_tmp != checksum)
    {
        qWarning() << "Fwupd Error: " << filename_cache << " does not match checksum, expected" << checksum_tmp << "got" << checksum;
        QFile::remove(filename_cache);
        return nullptr;
    }

    /* link file to application and return its reference */
    app->setFile(filename_cache);
    if (!app->needsReboot())
        app->setState(AbstractResource::Upgradeable);
    return app;
}

bool FwupdBackend::downloadFile(const QUrl &uri, const QString &filename)
{
    Q_ASSERT(QThread::currentThread() != QCoreApplication::instance()->thread());

    QScopedPointer<QNetworkAccessManager> manager(new QNetworkAccessManager);
    QEventLoop loop;
    QTimer getTimer;
    connect(&getTimer, &QTimer::timeout, &loop, &QEventLoop::quit);
    connect(manager.data(), &QNetworkAccessManager::finished, &loop, &QEventLoop::quit);
    QScopedPointer<QNetworkReply> reply(manager->get(QNetworkRequest(uri)));
    getTimer.start(600000); // 60 Seconds TimeOout Period
    loop.exec();
    if (!reply)
    {
        return false;
    } else if (QNetworkReply::NoError != reply->error() ) {
        qWarning() << "error fetching" << uri;
        return false;
    } else if (reply->error() == QNetworkReply::NoError) {
        QFile file(filename);
        if (file.open(QIODevice::WriteOnly)) {
            file.write(reply->readAll());
        } else {
            qWarning() << "Fwupd Error: Cannot Open File to write Data" << filename;
        }
    }
    return true;
}

void FwupdBackend::refreshRemote(FwupdBackend* backend, FwupdRemote* remote, quint64 cacheAge)
{
    if (!fwupd_remote_get_filename_cache_sig(remote))
    {
        qWarning() << "Fwupd Error: " << "Remote " << fwupd_remote_get_id(remote) << "has no cache signature!";
        return;
    }

    /* check cache age */
    if (cacheAge > 0)
    {
        const quint64 age = fwupd_remote_get_age(remote);
        if (age < cacheAge)
        {
//             qDebug() << "Fwupd Info:" << fwupd_remote_get_id(remote) << "is only" << age << "seconds old, so ignoring refresh! ";
            return;
        }
    }

    const QString cacheId = QStringLiteral("fwupd/remotes.d/%1").arg(QString::fromUtf8(fwupd_remote_get_id(remote)));
    const auto basenameSig = QString::fromUtf8(g_path_get_basename(fwupd_remote_get_filename_cache_sig(remote)));
    const QString filenameSig = cacheFile(cacheId, basenameSig);

    if (filenameSig.isEmpty())
        return;

    /* download the signature first*/
    const QUrl urlSig(QString::fromUtf8(fwupd_remote_get_metadata_uri_sig(remote)));
    const QString filenameSigTmp(filenameSig + QStringLiteral(".tmp"));

    if (!downloadFile(urlSig, filenameSigTmp)) {
        qWarning() << "failed to download" << urlSig;
        return;
    }
    Q_ASSERT(QFile::exists(filenameSigTmp));

    const auto checksum = fwupd_remote_get_checksum(remote);
    const QCryptographicHash::Algorithm hashAlgorithm = gchecksumToQChryptographicHash()[fwupd_checksum_guess_kind(checksum)];
    const QByteArray hash = getChecksum(filenameSigTmp, hashAlgorithm);

    const QByteArray oldHash = getChecksum(filenameSig, hashAlgorithm);
    if (oldHash == hash) {
        qDebug() << "remote hasn't changed:" << fwupd_remote_get_id(remote);
        QFile::remove(filenameSigTmp);
        return;
    }

    QFile::remove(filenameSig);

    if (!QFile::rename(filenameSigTmp, filenameSig)) {
        QFile::remove(filenameSigTmp);
        qWarning() << "Fwupd Error: cannot save remote signature" << filenameSigTmp << "to" << filenameSig;
        return;
    }
    QFile::remove(filenameSigTmp);

    const auto basename = QString::fromUtf8(g_path_get_basename(fwupd_remote_get_filename_cache(remote)));
    const QString filename = cacheFile(cacheId, basename);

    if (filename.isEmpty())
        return;

    qDebug() << "Fwupd Info: saving new firmware metadata to:" <<  filename;

    const QUrl url(QString::fromUtf8(fwupd_remote_get_metadata_uri(remote)));
    if (!downloadFile(url, filename))
    {
        qWarning() << "Fwupd Error: cannot download file:" << filename;
        return;
    }

    g_autoptr(GError) error = nullptr;
    if (!fwupd_client_update_metadata(backend->client, fwupd_remote_get_id(remote), filename.toUtf8().constData(), filenameSig.toUtf8().constData(), nullptr, &error))
    {
        backend->handleError(&error);
    }
}

void FwupdBackend::handleError(GError **perror)
{
    //TODO: localise the error message
    if ((*perror)->code != FWUPD_ERROR_NOTHING_TO_DO) {
        const QString msg = QString::fromUtf8((*perror)->message);
        QTimer::singleShot(0, this, [this, msg](){
            Q_EMIT passiveMessage(msg);
        });
    }
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

void FwupdBackend::checkForUpdates()
{
    if (m_fetching)
        return;

    auto fw = new QFutureWatcher<GPtrArray*>(this);
    fw->setFuture(QtConcurrent::run([this] () -> GPtrArray*
        {
            const uint cacheAge = (24*60*60); // Check for updates every day
            g_autoptr(GError) error = nullptr;

            /* get devices */
            GPtrArray* devices = fwupd_client_get_devices(client, nullptr, nullptr);


            g_autoptr(GPtrArray) remotes = fwupd_client_get_remotes(client, nullptr, &error);
            for(uint i = 0; remotes && i < remotes->len; i++)
            {
                FwupdRemote *remote = (FwupdRemote *)g_ptr_array_index(remotes, i);
                if (!fwupd_remote_get_enabled(remote))
                    continue;

                if (fwupd_remote_get_kind(remote) == FWUPD_REMOTE_KIND_LOCAL)
                    continue;

                refreshRemote(this, remote, cacheAge);
            }
            return devices;
        }
    ));
    connect(fw, &QFutureWatcher<GPtrArray*>::finished, this, [this, fw]() {
        m_fetching = true;
        emit fetchingChanged();

        auto devices = fw->result();
        for(uint i = 0; devices && i < devices->len; i++) {
            FwupdDevice *device = (FwupdDevice *) g_ptr_array_index(devices, i);

            addResourceToList(createDevice(device));
        }
        g_ptr_array_unref(devices);

        addUpdates();
        addHistoricalUpdates();

        m_fetching = false;
        emit fetchingChanged();
        emit initialized();
        fw->deleteLater();
    });
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

    auto stream = new ResultsStream(QStringLiteral("FwupdStream"));
    auto f = [this, stream, filter] () {
        QVector<AbstractResource*> ret;
        foreach(AbstractResource* r, m_resources) {
            if (r->state() < filter.state)
                continue;

            if (filter.search.isEmpty() || r->name().contains(filter.search, Qt::CaseInsensitive) || r->comment().contains(filter.search, Qt::CaseInsensitive)) {
                ret += r;
            }
        }
        if (!ret.isEmpty())
            Q_EMIT stream->resourcesFound(ret);
        stream->finish();
    };
    if (isFetching()) {
        connect(this, &FwupdBackend::initialized, stream, f);
    } else {
        QTimer::singleShot(0, this, f);
    }
    return stream;
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
