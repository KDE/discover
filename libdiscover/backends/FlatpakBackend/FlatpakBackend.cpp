/***************************************************************************
 *   Copyright © 2013 Aleix Pol Gonzalez <aleixpol@blue-systems.com>       *
 *   Copyright © 2017 Jan Grulich <jgrulich@redhat.com>                    *
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

#include "FlatpakBackend.h"
#include "FlatpakFetchDataJob.h"
#include "FlatpakResource.h"
#include "FlatpakSourcesBackend.h"
#include "FlatpakTransaction.h"

#include <resources/StandardBackendUpdater.h>
#include <resources/SourcesModel.h>
#include <Transaction/Transaction.h>
#include <appstream/OdrsReviewsBackend.h>
#include <appstream/AppStreamIntegration.h>

#include <AppStreamQt/bundle.h>
#include <AppStreamQt/component.h>
#include <AppStreamQt/icon.h>
#include <AppStreamQt/metadata.h>

#include <KAboutData>
#include <KLocalizedString>
#include <KPluginFactory>
#include <KConfigGroup>
#include <KSharedConfig>

#include <QAction>
#include <QtConcurrentRun>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFutureWatcher>
#include <QMessageBox>
#include <QSettings>
#include <QThread>
#include <QTimer>
#include <QTextStream>
#include <QTemporaryFile>
#include <QNetworkAccessManager>

#include <glib.h>

MUON_BACKEND_PLUGIN(FlatpakBackend)

static QString idForInstalledRef(FlatpakInstallation *installation, FlatpakInstalledRef *ref)
{
    const FlatpakResource::ResourceType appType = flatpak_ref_get_kind(FLATPAK_REF(ref)) == FLATPAK_REF_KIND_APP ? FlatpakResource::DesktopApp : FlatpakResource::Runtime;
    const QString name = QLatin1String(flatpak_ref_get_name(FLATPAK_REF(ref)));
    const QString appId = appType == FlatpakResource::DesktopApp ? QLatin1String(flatpak_ref_get_name(FLATPAK_REF(ref))) + QStringLiteral(".desktop") : name;

    const QString arch = QString::fromUtf8(flatpak_ref_get_arch(FLATPAK_REF(ref)));
    const QString branch = QString::fromUtf8(flatpak_ref_get_branch(FLATPAK_REF(ref)));

    return QStringLiteral("%1/%2/%3/%4/%5/%6").arg(FlatpakResource::installationPath(installation), QLatin1String("flatpak"), QString::fromUtf8(flatpak_installed_ref_get_origin(ref)),
                                                   FlatpakResource::typeAsString(appType), appId, branch);
}

FlatpakBackend::FlatpakBackend(QObject* parent)
    : AbstractResourcesBackend(parent)
    , m_updater(new StandardBackendUpdater(this))
    , m_reviews(AppStreamIntegration::global()->reviews())
    , m_fetching(false)
    , m_refreshAppstreamMetadataJobs(0)
    , m_threadPool(new QThreadPool(this))
{
    g_autoptr(GError) error = nullptr;
    m_cancellable = g_cancellable_new();

    connect(m_updater, &StandardBackendUpdater::updatesCountChanged, this, &FlatpakBackend::updatesCountChanged);

    // Load flatpak installation
    if (!setupFlatpakInstallations(&error)) {
        qWarning() << "Failed to setup flatpak installations:" << error->message;
    } else {
        loadAppsFromAppstreamData();

        m_sources = new FlatpakSourcesBackend(m_installations, this);
        SourcesModel::global()->addSourcesBackend(m_sources);
    }

    connect(m_reviews.data(), &OdrsReviewsBackend::ratingsReady, this, &FlatpakBackend::announceRatingsReady);
}

FlatpakBackend::~FlatpakBackend()
{
    m_threadPool.clear();
    for(auto inst : m_installations)
        g_object_unref(inst);

    g_object_unref(m_cancellable);
}

bool FlatpakBackend::isValid() const
{
    return m_sources && !m_installations.isEmpty();
}

void FlatpakBackend::announceRatingsReady()
{
    emitRatingsReady();

    const auto ids = m_reviews->appstreamIds().toSet();
    foreach(AbstractResource* res, m_resources) {
        if (ids.contains(res->appstreamId())) {
            res->ratingFetched();
        }
    }
}

class FlatpakFetchRemoteResourceJob : public QNetworkAccessManager
{
Q_OBJECT
public:
    FlatpakFetchRemoteResourceJob(const QUrl &url, FlatpakBackend *backend)
        : QNetworkAccessManager(backend)
        , m_backend(backend)
        , m_url(url)
    {
    }

    void start()
    {
        auto replyGet = get(QNetworkRequest(m_url));

        connect(replyGet, &QNetworkReply::finished, this, [this, replyGet] {
            const QUrl originalUrl = replyGet->request().url();
            if (replyGet->error() != QNetworkReply::NoError) {
                qWarning() << "couldn't download" << originalUrl << replyGet->errorString();
                Q_EMIT jobFinished(false, nullptr);
                return;
            }

            const QUrl fileUrl = QUrl::fromLocalFile(QStandardPaths::writableLocation(QStandardPaths::TempLocation) + QLatin1Char('/') + originalUrl.fileName());
            auto replyPut = put(QNetworkRequest(fileUrl), replyGet->readAll());
            connect(replyPut, &QNetworkReply::finished, this, [this, originalUrl, fileUrl, replyPut]() {
                if (replyPut->error() == QNetworkReply::NoError) {
                    auto res = m_backend->resourceForFile(fileUrl);
                    if (res) {
                        FlatpakResource *resource = qobject_cast<FlatpakResource*>(res);
                        resource->setResourceFile(originalUrl);
                        Q_EMIT jobFinished(true, resource);
                    } else {
                        qWarning() << "couldn't download" << originalUrl << "into" << fileUrl << replyPut->errorString();
                        Q_EMIT jobFinished(false, nullptr);
                    }
                }
            });
        });
    }

Q_SIGNALS:
    void jobFinished(bool success, FlatpakResource *resource);

private:
    FlatpakBackend *m_backend;
    QUrl m_url;
};

FlatpakRemote * FlatpakBackend::getFlatpakRemoteByUrl(const QString &url, FlatpakInstallation *installation) const
{
    auto remotes = flatpak_installation_list_remotes(installation, m_cancellable, nullptr);
    if (!remotes) {
        return nullptr;
    }

    const QByteArray comparableUrl = url.toUtf8();
    for (uint i = 0; i < remotes->len; i++) {
        FlatpakRemote *remote = FLATPAK_REMOTE(g_ptr_array_index(remotes, i));

        if (comparableUrl == flatpak_remote_get_url(remote)) {
            return remote;
        }
    }
    return nullptr;
}

FlatpakInstalledRef * FlatpakBackend::getInstalledRefForApp(FlatpakInstallation *flatpakInstallation, FlatpakResource *resource) const
{
    FlatpakInstalledRef *ref = nullptr;
    g_autoptr(GError) localError = nullptr;

    if (!flatpakInstallation) {
        return ref;
    }

    const auto type = resource->type() == FlatpakResource::DesktopApp ? FLATPAK_REF_KIND_APP : FLATPAK_REF_KIND_RUNTIME;

    return flatpak_installation_get_installed_ref(flatpakInstallation,
                                                 type,
                                                 resource->flatpakName().toUtf8().constData(),
                                                 resource->arch().toUtf8().constData(),
                                                 resource->branch().toUtf8().constData(),
                                                 m_cancellable, &localError);
}

FlatpakResource * FlatpakBackend::getAppForInstalledRef(FlatpakInstallation *flatpakInstallation, FlatpakInstalledRef *ref) const
{
    return m_resources.value(idForInstalledRef(flatpakInstallation, ref));
}

FlatpakResource * FlatpakBackend::getRuntimeForApp(FlatpakResource *resource) const
{
    FlatpakResource *runtime = nullptr;
    const auto runtimeInfo = resource->runtime().split(QLatin1Char('/'));

    if (runtimeInfo.count() != 3) {
        return runtime;
    }

    const QString runtimeId = QStringLiteral("runtime/") + runtimeInfo.at(0) + QLatin1Char('/') + runtimeInfo.at(2);

    for(auto it = m_resources.constBegin(), itEnd = m_resources.constEnd(); it!=itEnd; ++it) {
        if (it.key().endsWith(runtimeId)) {
            runtime = *it;
            break;
        }
    }

    // TODO if runtime wasn't found, create a new one from available info

    return runtime;
}

FlatpakResource * FlatpakBackend::addAppFromFlatpakBundle(const QUrl &url)
{
    g_autoptr(GBytes) appstreamGz = nullptr;
    g_autoptr(GError) localError = nullptr;
    g_autoptr(GFile) file = nullptr;
    g_autoptr(FlatpakBundleRef) bundleRef = nullptr;
    AppStream::Component asComponent;

    file = g_file_new_for_path(url.toLocalFile().toUtf8().constData());
    bundleRef = flatpak_bundle_ref_new(file, &localError);

    if (!bundleRef) {
        qWarning() << "Failed to load bundle:" << localError->message;
        return nullptr;
    }

    appstreamGz = flatpak_bundle_ref_get_appstream(bundleRef);
    if (appstreamGz) {
        g_autoptr(GZlibDecompressor) decompressor = nullptr;
        g_autoptr(GInputStream) streamGz = nullptr;
        g_autoptr(GInputStream) streamData = nullptr;
        g_autoptr(GBytes) appstream = nullptr;

        /* decompress data */
        decompressor = g_zlib_decompressor_new (G_ZLIB_COMPRESSOR_FORMAT_GZIP);
        streamGz = g_memory_input_stream_new_from_bytes (appstreamGz);
        if (!streamGz) {
            return nullptr;
        }

        streamData = g_converter_input_stream_new (streamGz, G_CONVERTER (decompressor));

        appstream = g_input_stream_read_bytes (streamData, 0x100000, m_cancellable, &localError);
        if (!appstream) {
            qWarning() << "Failed to extract appstream metadata from bundle:" << localError->message;
            return nullptr;
        }

        gsize len = 0;
        gconstpointer data = g_bytes_get_data(appstream, &len);

        AppStream::Metadata metadata;
        metadata.setFormatStyle(AppStream::Metadata::FormatStyleCollection);
        AppStream::Metadata::MetadataError error = metadata.parse(QString::fromUtf8((char*)data, len), AppStream::Metadata::FormatKindXml);
        if (error != AppStream::Metadata::MetadataErrorNoError) {
            qWarning() << "Failed to parse appstream metadata: " << error;
            return nullptr;
        }

        QList<AppStream::Component> components = metadata.components();
        if (components.size()) {
            asComponent = AppStream::Component(components.first());
        } else {
            qWarning() << "Failed to parse appstream metadata";
            return nullptr;
        }
    } else {
        asComponent = AppStream::Component();
        qWarning() << "No appstream metadata in bundle";
    }

    gsize len = 0;
    g_autoptr(GBytes) iconData = nullptr;
    g_autoptr(GBytes) metadata = nullptr;
    FlatpakResource *resource = new FlatpakResource(asComponent, preferredInstallation(), this);

    metadata = flatpak_bundle_ref_get_metadata(bundleRef);
    QByteArray metadataContent = QByteArray((char *)g_bytes_get_data(metadata, &len));
    if (!updateAppMetadata(resource, metadataContent)) {
        delete resource;
        qWarning() << "Failed to update metadata from app bundle";
        return nullptr;
    }

    iconData = flatpak_bundle_ref_get_icon(bundleRef, 128);
    if (!iconData) {
        iconData = flatpak_bundle_ref_get_icon(bundleRef, 64);
    }

    if (iconData) {
        gsize len = 0;
        QPixmap pixmap;
        char * data = (char *)g_bytes_get_data(iconData, &len);
        QByteArray icon = QByteArray(data, len);
        pixmap.loadFromData(icon, "PNG");
        resource->setBundledIcon(pixmap);
    }

    const QString origin = QString::fromUtf8(flatpak_bundle_ref_get_origin(bundleRef));

    resource->setDownloadSize(0);
    resource->setInstalledSize(flatpak_bundle_ref_get_installed_size(bundleRef));
    resource->setPropertyState(FlatpakResource::DownloadSize, FlatpakResource::AlreadyKnown);
    resource->setPropertyState(FlatpakResource::InstalledSize, FlatpakResource::AlreadyKnown);
    resource->setFlatpakFileType(QStringLiteral("flatpak"));
    resource->setOrigin(origin.isEmpty() ? i18n("Local bundle") : origin);
    resource->setResourceFile(url);
    resource->setState(FlatpakResource::None);
    resource->setType(FlatpakResource::DesktopApp);

    addResource(resource);
    return resource;
}

FlatpakResource * FlatpakBackend::addAppFromFlatpakRef(const QUrl &url)
{
    QSettings settings(url.toLocalFile(), QSettings::NativeFormat);
    const QString refurl = settings.value(QStringLiteral("Flatpak Ref/Url")).toString();

    g_autoptr(GError) error = NULL;
    g_autoptr(FlatpakRemoteRef) remoteRef = nullptr;
    {
        QFile f(url.toLocalFile());
        if (!f.open(QFile::ReadOnly | QFile::Text)) {
            return nullptr;
        }

        QByteArray contents = f.readAll();

        g_autoptr(GBytes) bytes = g_bytes_new (contents.data(), contents.size());

        remoteRef = flatpak_installation_install_ref_file (preferredInstallation(), bytes, m_cancellable, &error);
        if (!remoteRef) {
            qWarning() << "Failed to install ref file:" << error->message;
            return nullptr;
        }
    }

    const auto remoteName = flatpak_remote_ref_get_remote_name(remoteRef);

    auto ref = FLATPAK_REF(remoteRef);

    AppStream::Component asComponent;
    asComponent.addUrl(AppStream::Component::UrlKindHomepage, settings.value(QStringLiteral("Flatpak Ref/Homepage")).toString());
    asComponent.setDescription(settings.value(QStringLiteral("Flatpak Ref/Description")).toString());
    asComponent.setName(settings.value(QStringLiteral("Flatpak Ref/Title")).toString());
    asComponent.setSummary(settings.value(QStringLiteral("Flatpak Ref/Comment")).toString());
    asComponent.setId(settings.value(QStringLiteral("Flatpak Ref/Name")).toString());

    const QString iconUrl = settings.value(QStringLiteral("Flatpak Ref/Icon")).toString();
    if (!iconUrl.isEmpty()) {
        AppStream::Icon icon;
        icon.setKind(AppStream::Icon::KindRemote);
        icon.setUrl(QUrl(iconUrl));
        asComponent.addIcon(icon);
    }

    auto resource = new FlatpakResource(asComponent, preferredInstallation(), this);
    resource->setFlatpakFileType(QStringLiteral("flatpakref"));
    resource->setOrigin(QString::fromUtf8(remoteName));
    resource->updateFromRef(ref);

    QUrl runtimeUrl = QUrl(settings.value(QStringLiteral("Flatpak Ref/RuntimeRepo")).toString());
    if (!runtimeUrl.isEmpty()) {
        auto installation = preferredInstallation();
        // We need to fetch metadata to find information about required runtime
        auto fw = new QFutureWatcher<QByteArray>(this);
        fw->setFuture(QtConcurrent::run(&m_threadPool, &FlatpakRunnables::fetchMetadata, installation, resource));
        connect(fw, &QFutureWatcher<QByteArray>::finished, this, [this, installation, resource, fw, runtimeUrl]() {
            const auto metadata = fw->result();
            // Even when we failed to fetch information about runtime we still want to show the application
            if (metadata.isEmpty()) {
                onFetchMetadataFinished(installation, resource, metadata);
            } else {
                updateAppMetadata(resource, metadata);

                auto runtime = getRuntimeForApp(resource);
                if (!runtime || (runtime && !runtime->isInstalled())) {
                    FlatpakFetchRemoteResourceJob *fetchRemoteResource = new FlatpakFetchRemoteResourceJob(runtimeUrl, this);
                    connect(fetchRemoteResource, &FlatpakFetchRemoteResourceJob::jobFinished, this, [this, resource] (bool success, FlatpakResource *repoResource) {
                        if (success) {
                            installApplication(repoResource);
                        }
                        addResource(resource);
                    });
                    fetchRemoteResource->start();
                    return;
                } else {
                    addResource(resource);
                }
            }
            fw->deleteLater();
        });
    } else {
        addResource(resource);
    }

    return resource;
}

FlatpakResource * FlatpakBackend::addSourceFromFlatpakRepo(const QUrl &url)
{
    Q_ASSERT(url.isLocalFile());
    QSettings settings(url.toLocalFile(), QSettings::NativeFormat);

    const QString gpgKey = settings.value(QStringLiteral("Flatpak Repo/GPGKey")).toString();
    const QString title = settings.value(QStringLiteral("Flatpak Repo/Title")).toString();
    const QString repoUrl = settings.value(QStringLiteral("Flatpak Repo/Url")).toString();

    if (gpgKey.isEmpty() || title.isEmpty() || repoUrl.isEmpty()) {
        return nullptr;
    }

    if (gpgKey.startsWith(QStringLiteral("http://")) || gpgKey.startsWith(QStringLiteral("https://"))) {
        return nullptr;
    }

    AppStream::Component asComponent;
    asComponent.addUrl(AppStream::Component::UrlKindHomepage, settings.value(QStringLiteral("Flatpak Repo/Homepage")).toString());
    asComponent.setSummary(settings.value(QStringLiteral("Flatpak Repo/Comment")).toString());
    asComponent.setDescription(settings.value(QStringLiteral("Flatpak Repo/Description")).toString());
    asComponent.setName(title);
    asComponent.setId(settings.value(QStringLiteral("Flatpak Ref/Name")).toString());

    const QString iconUrl = settings.value(QStringLiteral("Flatpak Repo/Icon")).toString();
    if (!iconUrl.isEmpty()) {
        AppStream::Icon icon;
        icon.setKind(AppStream::Icon::KindRemote);
        icon.setUrl(QUrl(iconUrl));
        asComponent.addIcon(icon);
    }

    auto resource = new FlatpakResource(asComponent, preferredInstallation(), this);
    // Use metadata only for stuff which are not common for all resources
    resource->addMetadata(QStringLiteral("gpg-key"), gpgKey);
    resource->addMetadata(QStringLiteral("repo-url"), repoUrl);
    resource->setBranch(settings.value(QStringLiteral("Flatpak Repo/DefaultBranch")).toString());
    resource->setFlatpakName(url.fileName().remove(QStringLiteral(".flatpakrepo")));
    resource->setType(FlatpakResource::Source);

    auto repo = flatpak_installation_get_remote_by_name(preferredInstallation(), resource->flatpakName().toUtf8().constData(), m_cancellable, nullptr);
    if (!repo) {
        resource->setState(AbstractResource::State::None);
    } else {
        resource->setState(AbstractResource::State::Installed);
    }

    return resource;
}

void FlatpakBackend::addResource(FlatpakResource *resource)
{
    // Update app with all possible information we have
    if (!parseMetadataFromAppBundle(resource)) {
        qWarning() << "Failed to parse metadata from app bundle for" << resource->name();
    }

    auto installation = resource->installation();
    updateAppState(installation, resource);

    // This will update also metadata (required runtime)
    updateAppSize(installation, resource);

    connect(resource, &FlatpakResource::stateChanged, this, &FlatpakBackend::updatesCountChanged);

    m_resources.insert(resource->uniqueId(), resource);
}

bool FlatpakBackend::compareAppFlatpakRef(FlatpakInstallation *flatpakInstallation, FlatpakResource *resource, FlatpakInstalledRef *ref) const
{
    return resource->uniqueId() == idForInstalledRef(flatpakInstallation, ref);
}

class FlatpakSource
{
public:
    FlatpakSource(FlatpakRemote* remote) : m_remote(remote) {}

    bool isEnabled() const
    {
        return !flatpak_remote_get_disabled(m_remote);
    }

    QString appstreamDir() const
    {
        g_autoptr(GFile) appstreamDir = flatpak_remote_get_appstream_dir(m_remote, nullptr);
        if (!appstreamDir) {
            qWarning() << "No appstream dir for" << flatpak_remote_get_name(m_remote);
            return {};
        }
        return QString::fromUtf8(g_file_get_path(appstreamDir));
    }

    QString name() const
    {
        return QString::fromUtf8(flatpak_remote_get_name(m_remote));
    }

private:
    FlatpakRemote* m_remote;
};

void FlatpakBackend::finishInitialization()
{
    loadInstalledApps();
    checkForUpdates();
}

void FlatpakBackend::loadAppsFromAppstreamData()
{
    for (auto installation : qAsConst(m_installations)) {
        // Load applications from appstream metadata
        if (!loadAppsFromAppstreamData(installation)) {
            qWarning() << "Failed to load packages from appstream data from installation" << installation;
        }
    }
}

bool FlatpakBackend::loadAppsFromAppstreamData(FlatpakInstallation *flatpakInstallation)
{
    Q_ASSERT(flatpakInstallation);

    GPtrArray *remotes = flatpak_installation_list_remotes(flatpakInstallation, m_cancellable, nullptr);
    if (!remotes) {
        return false;
    }

    m_refreshAppstreamMetadataJobs += remotes->len;

    for (uint i = 0; i < remotes->len; i++) {
        FlatpakRemote *remote = FLATPAK_REMOTE(g_ptr_array_index(remotes, i));
        g_autoptr(GFile) fileTimestamp = flatpak_remote_get_appstream_timestamp(remote, nullptr);

        QFileInfo fileInfo = QFileInfo(QString::fromUtf8(g_file_get_path(fileTimestamp)));
        // Refresh appstream metadata in case they have never been refreshed or the cache is older than 6 hours
        if (!fileInfo.exists() || fileInfo.lastModified().toUTC().secsTo(QDateTime::currentDateTimeUtc()) > 21600) {
            refreshAppstreamMetadata(flatpakInstallation, remote);
        } else {
            integrateRemote(flatpakInstallation, remote);
        }
    }

    return true;
}

void FlatpakBackend::integrateRemote(FlatpakInstallation *flatpakInstallation, FlatpakRemote *remote)
{
    m_refreshAppstreamMetadataJobs--;

    FlatpakSource source(remote);
    if (!source.isEnabled() || flatpak_remote_get_noenumerate(remote)) {
        return;
    }

    const QString appstreamDirPath = source.appstreamDir();
    const QString appstreamIconsPath = source.appstreamDir() + QLatin1String("/icons/");
    const QString appDirFileName = appstreamDirPath + QLatin1String("/appstream.xml.gz");
    if (!QFile::exists(appDirFileName)) {
        qWarning() << "No" << appDirFileName << "appstream metadata found for" << source.name();
        return;
    }

    AppStream::Metadata metadata;
    metadata.setFormatStyle(AppStream::Metadata::FormatStyleCollection);
    AppStream::Metadata::MetadataError error = metadata.parseFile(appDirFileName, AppStream::Metadata::FormatKindXml);
    if (error != AppStream::Metadata::MetadataErrorNoError) {
        qWarning() << "Failed to parse appstream metadata: " << error;
        return;
    }

    QList<AppStream::Component> components = metadata.components();
    foreach (const AppStream::Component& appstreamComponent, components) {
        FlatpakResource *resource = new FlatpakResource(appstreamComponent, flatpakInstallation, this);
        resource->setIconPath(appstreamIconsPath);
        resource->setOrigin(source.name());
        addResource(resource);
    }

    // Not ideal calling it from here but given that everything is asynchronous then it's a bit complicated
    if (!m_refreshAppstreamMetadataJobs) {
        finishInitialization();
    }
}

void FlatpakBackend::loadInstalledApps()
{
    for (auto installation : qAsConst(m_installations)) {
        // Load installed applications and update existing resources with info from installed application
        if (!loadInstalledApps(installation)) {
            qWarning() << "Failed to load installed packages from installation" << installation;
        }
    }
}

bool FlatpakBackend::loadInstalledApps(FlatpakInstallation *flatpakInstallation)
{
    Q_ASSERT(flatpakInstallation);

    g_autoptr(GError) localError = nullptr;
    g_autoptr(GPtrArray) refs = flatpak_installation_list_installed_refs(flatpakInstallation, m_cancellable, &localError);
    if (!refs) {
        qWarning() << "Failed to get list of installed refs for listing updates:" << localError->message;
        return false;
    }

    const QString pathExports = FlatpakResource::installationPath(flatpakInstallation) + QLatin1String("/exports/");
    const QString pathApps = pathExports + QLatin1String("share/applications/");

    for (uint i = 0; i < refs->len; i++) {
        FlatpakInstalledRef *ref = FLATPAK_INSTALLED_REF(g_ptr_array_index(refs, i));

        if (flatpak_ref_get_kind(FLATPAK_REF(ref)) == FLATPAK_REF_KIND_RUNTIME) {
            continue;
        }

        const auto name = QLatin1String(flatpak_ref_get_name(FLATPAK_REF(ref)));
        const QString fnDesktop = pathApps + name + QLatin1String(".desktop");

        if (!QFileInfo::exists(fnDesktop)) {
            continue;
        }

        AppStream::Metadata metadata;
        AppStream::Metadata::MetadataError error = metadata.parseFile(fnDesktop, AppStream::Metadata::FormatKindDesktopEntry);
        if (error != AppStream::Metadata::MetadataErrorNoError) {
            qWarning() << "Failed to parse appstream metadata: " << error << fnDesktop;
            continue;
        }

        AppStream::Component appstreamComponent(metadata.component());
        FlatpakResource *resource = new FlatpakResource(appstreamComponent, flatpakInstallation, this);

        resource->setIconPath(pathExports);
        resource->setState(AbstractResource::Installed);
        resource->setOrigin(QString::fromUtf8(flatpak_installed_ref_get_origin(ref)));
        resource->updateFromRef(FLATPAK_REF(ref));

        // Go through apps we already know about from appstream metadata
        bool resourceExists = false;
        foreach (FlatpakResource *res, m_resources) {
            // Compare the only information we have
            if (res->appstreamId() == QStringLiteral("%1.desktop").arg(resource->appstreamId()) && res->name() == resource->name()) {
                resourceExists = true;
                res->setState(resource->state());
                break;
            }
        }

        if (!resourceExists) {
            addResource(resource);
        } else {
            resource->deleteLater();
        }
    }

    return true;
}

void FlatpakBackend::loadLocalUpdates(FlatpakInstallation *flatpakInstallation)
{
    g_autoptr(GError) localError = nullptr;
    g_autoptr(GPtrArray) refs = nullptr;

    refs = flatpak_installation_list_installed_refs(flatpakInstallation, m_cancellable, &localError);
    if (!refs) {
        qWarning() << "Failed to get list of installed refs for listing updates:" << localError->message;
        return;
    }

    for (uint i = 0; i < refs->len; i++) {
        FlatpakInstalledRef *ref = FLATPAK_INSTALLED_REF(g_ptr_array_index(refs, i));

        const gchar *latestCommit = flatpak_installed_ref_get_latest_commit(ref);

        if (!latestCommit) {
            qWarning() << "Couldn't get latest commit for" << flatpak_ref_get_name(FLATPAK_REF(ref));
            continue;
        }

        const gchar *commit = flatpak_ref_get_commit(FLATPAK_REF(ref));
        if (g_strcmp0(commit, latestCommit) == 0) {
            continue;
        }

        FlatpakResource *resource = getAppForInstalledRef(flatpakInstallation, ref);
        if (resource) {
            resource->setState(AbstractResource::Upgradeable);
            updateAppSize(flatpakInstallation, resource);
        }
    }
}

void FlatpakBackend::loadRemoteUpdates(FlatpakInstallation* installation)
{
    auto fw = new QFutureWatcher<GPtrArray *>(this);
    fw->setFuture(QtConcurrent::run(&m_threadPool, [installation]() -> GPtrArray * {
        g_autoptr(GCancellable) cancellable = g_cancellable_new();
        g_autoptr(GError) localError = nullptr;
        GPtrArray *refs = flatpak_installation_list_installed_refs_for_update(installation, cancellable, &localError);
        if (!refs) {
            qWarning() << "Failed to get list of installed refs for listing updates: " << localError->message;
        }
        return refs;
    }));
    connect(fw, &QFutureWatcher<GPtrArray *>::finished, this, [this, installation, fw](){
        auto refs = fw->result();
        onFetchUpdatesFinished(installation, refs);
        fw->deleteLater();
    });
}

void FlatpakBackend::onFetchUpdatesFinished(FlatpakInstallation *flatpakInstallation, GPtrArray *updates)
{
    g_autoptr(GPtrArray) fetchedUpdates = updates;

    for (uint i = 0; i < fetchedUpdates->len; i++) {
        FlatpakInstalledRef *ref = FLATPAK_INSTALLED_REF(g_ptr_array_index(fetchedUpdates, i));
        FlatpakResource *resource = getAppForInstalledRef(flatpakInstallation, ref);
        if (resource) {
            resource->setState(AbstractResource::Upgradeable);
            updateAppSize(flatpakInstallation, resource);
        }
    }
}

bool FlatpakBackend::parseMetadataFromAppBundle(FlatpakResource *resource)
{
    g_autoptr(FlatpakRef) ref = nullptr;
    g_autoptr(GError) localError = nullptr;
    AppStream::Bundle bundle = resource->appstreamComponent().bundle(AppStream::Bundle::KindFlatpak);

    // Get arch/branch/commit/name from FlatpakRef
    if (!bundle.isEmpty()) {
        ref = flatpak_ref_parse(bundle.id().toUtf8().constData(), &localError);
        if (!ref) {
            qWarning() << "Failed to parse" << bundle.id() << localError->message;
            return false;
        } else {
            resource->updateFromRef(ref);
        }
    }

    return true;
}

class FlatpakRefreshAppstreamMetadataJob : public QThread
{
    Q_OBJECT
public:
    FlatpakRefreshAppstreamMetadataJob(FlatpakInstallation *installation, FlatpakRemote *remote)
        : QThread()
        , m_cancellable(g_cancellable_new())
        , m_installation(installation)
        , m_remote(remote)
    {
        connect(this, &FlatpakRefreshAppstreamMetadataJob::finished, this, &QObject::deleteLater);
    }

    ~FlatpakRefreshAppstreamMetadataJob()
    {
        g_object_unref(m_cancellable);
    }

    void cancel()
    {
        g_cancellable_cancel(m_cancellable);
    }

    void run() override
    {
        g_autoptr(GError) localError = nullptr;

#if FLATPAK_CHECK_VERSION(0,9,4)
        // With Flatpak 0.9.4 we can use flatpak_installation_update_appstream_full_sync() providing progress reporting which we don't use at this moment, but still
        // better to use newer function in case the previous one gets deprecated
        if (!flatpak_installation_update_appstream_full_sync(m_installation, flatpak_remote_get_name(m_remote), nullptr, nullptr, nullptr, nullptr, m_cancellable, &localError)) {
#else
        if (!flatpak_installation_update_appstream_sync(m_installation, flatpak_remote_get_name(m_remote), nullptr, nullptr, m_cancellable, &localError)) {
#endif
            qWarning() << "Failed to refresh appstream metadata for " << flatpak_remote_get_name(m_remote) << ": " << (localError ? localError->message : "<no error>");
            Q_EMIT jobRefreshAppstreamMetadataFailed();
        } else  {
            Q_EMIT jobRefreshAppstreamMetadataFinished(m_installation, m_remote);
        }
    }

Q_SIGNALS:
    void jobRefreshAppstreamMetadataFailed();
    void jobRefreshAppstreamMetadataFinished(FlatpakInstallation *installation, FlatpakRemote *remote);

private:
    GCancellable *m_cancellable;
    FlatpakInstallation *m_installation;
    FlatpakRemote *m_remote;
};

void FlatpakBackend::refreshAppstreamMetadata(FlatpakInstallation *installation, FlatpakRemote *remote)
{
    FlatpakRefreshAppstreamMetadataJob *job = new FlatpakRefreshAppstreamMetadataJob(installation, remote);
    connect(job, &FlatpakRefreshAppstreamMetadataJob::jobRefreshAppstreamMetadataFailed, this, [this] () {
        m_refreshAppstreamMetadataJobs--;
    });
    connect(job, &FlatpakRefreshAppstreamMetadataJob::jobRefreshAppstreamMetadataFinished, this, &FlatpakBackend::integrateRemote);
    job->start();
}

bool FlatpakBackend::setupFlatpakInstallations(GError **error)
{
    GPtrArray *installations = flatpak_get_system_installations(m_cancellable, error);
    if (*error) {
        qWarning() << "Failed to call flatpak_get_system_installations:" << (*error)->message;
    }
    for (uint i = 0; installations && i < installations->len; i++) {
        m_installations << FLATPAK_INSTALLATION(g_ptr_array_index(installations, i));
    }

    auto user = flatpak_installation_new_user(m_cancellable, error);
    if (user) {
        m_installations << user;
    }

    return !m_installations.isEmpty();
}

void FlatpakBackend::updateAppInstalledMetadata(FlatpakInstalledRef *installedRef, FlatpakResource *resource)
{
    // Update the rest
    resource->updateFromRef(FLATPAK_REF(installedRef));
    resource->setInstalledSize(flatpak_installed_ref_get_installed_size(installedRef));
    resource->setOrigin(QString::fromUtf8(flatpak_installed_ref_get_origin(installedRef)));
    resource->setState(AbstractResource::Installed);
}

bool FlatpakBackend::updateAppMetadata(FlatpakInstallation* flatpakInstallation, FlatpakResource *resource)
{
    QByteArray metadataContent;
    g_autoptr(GFile) installationPath = nullptr;

    if (resource->type() != FlatpakResource::DesktopApp) {
        return true;
    }

    installationPath = flatpak_installation_get_path(flatpakInstallation);
    const QString path = QString::fromUtf8(g_file_get_path(installationPath)) + QStringLiteral("/app/%1/%2/%3/active/metadata").arg(resource->flatpakName()).arg(resource->arch()).arg(resource->branch());

    if (QFile::exists(path)) {
        return updateAppMetadata(resource, path);
    } else {
        auto fw = new QFutureWatcher<QByteArray>(this);
        fw->setFuture(QtConcurrent::run(&m_threadPool, &FlatpakRunnables::fetchMetadata, flatpakInstallation, resource));
        connect(fw, &QFutureWatcher<QByteArray>::finished, this, [this, flatpakInstallation, resource, fw]() {
            const auto metadata = fw->result();
            if (!metadata.isEmpty())
                onFetchMetadataFinished(flatpakInstallation, resource, metadata);
            fw->deleteLater();
        });

        // Return false to indicate we cannot continue (right now used only in updateAppSize())
        return false;
    }
}

void FlatpakBackend::onFetchMetadataFinished(FlatpakInstallation *flatpakInstallation, FlatpakResource *resource, const QByteArray &metadata)
{
    updateAppMetadata(resource, metadata);

    // Right now we attempt to update metadata for calculating the size so call updateSizeFromRemote()
    // as it's what we want. In future if there are other reason to update metadata we will need to somehow
    // distinguish betwen these calls
    updateAppSizeFromRemote(flatpakInstallation, resource);
}

bool FlatpakBackend::updateAppMetadata(FlatpakResource *resource, const QString &path)
{
    // Parse the temporary file
    QSettings setting(path, QSettings::NativeFormat);
    setting.beginGroup(QLatin1String("Application"));
    // Set the runtime in form of name/arch/version which can be later easily parsed
    resource->setRuntime(setting.value(QLatin1String("runtime")).toString());
    // TODO get more information?
    return true;
}

bool FlatpakBackend::updateAppMetadata(FlatpakResource *resource, const QByteArray &data)
{
    // Save the content to temporary file
    QTemporaryFile tempFile;
    tempFile.setAutoRemove(false);
    if (!tempFile.open()) {
        qWarning() << "Failed to get metadata file";
        return false;
    }

    tempFile.write(data);
    tempFile.close();

    updateAppMetadata(resource, tempFile.fileName());

    tempFile.remove();

    return true;
}

bool FlatpakBackend::updateAppSize(FlatpakInstallation *flatpakInstallation, FlatpakResource *resource)
{
    // Check if the size is already set, we should also distiguish between download and installed size,
    // right now it doesn't matter whether we get size for installed or not installed app, but if we
    // start making difference then for not installed app check download and install size separately

    if (resource->state() == AbstractResource::Installed) {
        // The size appears to be already set (from updateAppInstalledMetadata() apparently)
        if (resource->installedSize() > 0) {
            return true;
        }
    } else {
        if (resource->installedSize() > 0 && resource->downloadSize() > 0) {
            return true;
        }
    }

    // Check if we know the needed runtime which is needed for calculating the size
    if (resource->runtime().isEmpty()) {
        if (!updateAppMetadata(flatpakInstallation, resource)) {
            return false;
        }
    }

    return updateAppSizeFromRemote(flatpakInstallation, resource);
}

bool FlatpakBackend::updateAppSizeFromRemote(FlatpakInstallation *flatpakInstallation, FlatpakResource *resource)
{
    // Calculate the runtime size
    if (resource->state() == AbstractResource::None && resource->type() == FlatpakResource::DesktopApp) {
        auto runtime = getRuntimeForApp(resource);
        if (runtime) {
            // Re-check runtime state if case a new one was created
            updateAppState(flatpakInstallation, runtime);

            if (!runtime->isInstalled()) {
                if (!updateAppSize(flatpakInstallation, runtime)) {
                    qWarning() << "Failed to get runtime size needed for total size of" << resource->name();
                    return false;
                }
                // Set required download size to include runtime size even now, in case we fail to
                // get the app size (e.g. when installing bundles where download size is 0)
                resource->setDownloadSize(runtime->downloadSize());
            }
        }
    }

    if (resource->state() == AbstractResource::Installed) {
        g_autoptr(FlatpakInstalledRef) ref = nullptr;
        ref = getInstalledRefForApp(flatpakInstallation, resource);
        if (!ref) {
            qWarning() << "Failed to get installed size of" << resource->name();
            return false;
        }
        resource->setInstalledSize(flatpak_installed_ref_get_installed_size(ref));
    } else {
        if (resource->origin().isEmpty()) {
            qWarning() << "Failed to get size of" << resource->name() << " because of missing origin";
            return false;
        }

        auto futureWatcher = new QFutureWatcher<FlatpakRunnables::SizeInformation>(this);
        futureWatcher->setFuture(QtConcurrent::run(&m_threadPool, &FlatpakRunnables::fetchFlatpakSize, flatpakInstallation, resource));
        connect(futureWatcher, &QFutureWatcher<FlatpakRunnables::SizeInformation>::finished, this, [this, resource, futureWatcher]() {
            auto value = futureWatcher->result();
            if (value.valid) {
                onFetchSizeFinished(resource, value.downloadSize, value.installedSize);
            } else {
                resource->setPropertyState(FlatpakResource::DownloadSize, FlatpakResource::UnknownOrFailed);
                resource->setPropertyState(FlatpakResource::InstalledSize, FlatpakResource::UnknownOrFailed);
            }
            futureWatcher->deleteLater();
        });
    }

    return true;
}

void FlatpakBackend::onFetchSizeFinished(FlatpakResource *resource, guint64 downloadSize, guint64 installedSize)
{
    FlatpakResource *runtime = nullptr;
    if (resource->state() == AbstractResource::None && resource->type() == FlatpakResource::DesktopApp) {
        runtime = getRuntimeForApp(resource);
    }

    if (runtime && !runtime->isInstalled()) {
        resource->setDownloadSize(runtime->downloadSize() + downloadSize);
        resource->setInstalledSize(installedSize);
    } else {
        resource->setDownloadSize(downloadSize);
        resource->setInstalledSize(installedSize);
    }
}

void FlatpakBackend::updateAppState(FlatpakInstallation *flatpakInstallation, FlatpakResource *resource)
{
    FlatpakInstalledRef *ref = getInstalledRefForApp(flatpakInstallation, resource);
    if (ref) {
        // If the app is installed, we can set information about commit, arch etc.
        updateAppInstalledMetadata(ref, resource);
    } else {
        // TODO check if the app is actuall still available
        resource->setState(AbstractResource::None);
    }
}

void FlatpakBackend::setFetching(bool fetching)
{
    if (m_fetching != fetching) {
        m_fetching = fetching;
        emit fetchingChanged();
    }
}

int FlatpakBackend::updatesCount() const
{
    return m_updater->updatesCount();
}

ResultsStream * FlatpakBackend::search(const AbstractResourcesBackend::Filters &filter)
{
    if (filter.resourceUrl.fileName().endsWith(QLatin1String(".flatpakrepo")) || filter.resourceUrl.fileName().endsWith(QLatin1String(".flatpakref"))) {
        auto stream = new ResultsStream(QStringLiteral("FlatpakStream-http-")+filter.resourceUrl.fileName());

        FlatpakFetchRemoteResourceJob *fetchResourceJob = new FlatpakFetchRemoteResourceJob(filter.resourceUrl, this);
        connect(fetchResourceJob, &FlatpakFetchRemoteResourceJob::jobFinished, this, [fetchResourceJob, stream] (bool success, FlatpakResource *resource) {
            if (success) {
                stream->resourcesFound({resource});
            }
            stream->finish();
            fetchResourceJob->deleteLater();
        });
        fetchResourceJob->start();

        return stream;
    } else if (!filter.resourceUrl.isEmpty() && filter.resourceUrl.scheme() != QLatin1String("appstream"))
        return new ResultsStream(QStringLiteral("FlatpakStream-void"), {});

    QVector<AbstractResource*> ret;
    foreach(AbstractResource* r, m_resources) {
        if (r->isTechnical() && filter.state != AbstractResource::Upgradeable) {
            continue;
        }

        if (!filter.resourceUrl.isEmpty() && filter.resourceUrl.host().compare(r->appstreamId(), Qt::CaseInsensitive) != 0)
            continue;

        if (r->state() < filter.state)
            continue;

        if (filter.search.isEmpty() || r->name().contains(filter.search, Qt::CaseInsensitive) || r->comment().contains(filter.search, Qt::CaseInsensitive)) {
            ret += r;
        }
    }
    return new ResultsStream(QStringLiteral("FlatpakStream"), ret);
}

ResultsStream * FlatpakBackend::findResourceByPackageName(const QUrl &url)
{
    QVector<AbstractResource*> resources;
    if (url.scheme() == QLatin1String("appstream")) {
        if (url.host().isEmpty())
            passiveMessage(i18n("Malformed appstream url '%1'", url.toDisplayString()));
        else {
            foreach(FlatpakResource* res, m_resources) {
                if (QString::compare(res->appstreamId(), url.host(), Qt::CaseInsensitive)==0)
                    resources << res;
            }
        }
    }
    return new ResultsStream(QStringLiteral("FlatpakStream"), resources);
}

AbstractBackendUpdater * FlatpakBackend::backendUpdater() const
{
    return m_updater;
}

AbstractReviewsBackend * FlatpakBackend::reviewsBackend() const
{
    return m_reviews.data();
}

Transaction* FlatpakBackend::installApplication(AbstractResource *app, const AddonList &addons)
{
    Q_UNUSED(addons);

    FlatpakResource *resource = qobject_cast<FlatpakResource*>(app);

    if (resource->type() == FlatpakResource::Source) {
        // Let source backend handle this
        FlatpakRemote *remote = m_sources->installSource(resource);
        if (remote) {
            resource->setState(AbstractResource::Installed);
            // Make sure we update appstream metadata first
            // FIXME we have to let flatpak to return the remote as the one created by FlatpakSourcesBackend will not have appstream directory
            refreshAppstreamMetadata(preferredInstallation(), flatpak_installation_get_remote_by_name(preferredInstallation(), flatpak_remote_get_name(remote), nullptr, nullptr));
        }
        return nullptr;
    }

    FlatpakTransaction *transaction = nullptr;
    FlatpakInstallation *installation = resource->installation();

    if (resource->propertyState(FlatpakResource::RequiredRuntime) == FlatpakResource::NotKnownYet && resource->type() == FlatpakResource::DesktopApp) {
        transaction = new FlatpakTransaction(resource, Transaction::InstallRole, true);
        connect(resource, &FlatpakResource::propertyStateChanged, [resource, transaction, this] (FlatpakResource::PropertyKind kind, FlatpakResource::PropertyState state) {
            if (kind != FlatpakResource::RequiredRuntime) {
                return;
            }

            if (state == FlatpakResource::AlreadyKnown) {
                FlatpakResource *runtime = getRuntimeForApp(resource);
                if (runtime && !runtime->isInstalled()) {
                    transaction->setRuntime(runtime);
                }
            }
            transaction->start();
        });
    } else {
        FlatpakResource *runtime = getRuntimeForApp(resource);
        if (runtime && !runtime->isInstalled()) {
            transaction = new FlatpakTransaction(resource, runtime, Transaction::InstallRole);
        } else {
            transaction = new FlatpakTransaction(resource, Transaction::InstallRole);
        }
    }

    connect(transaction, &FlatpakTransaction::statusChanged, [this, installation, resource] (Transaction::Status status) {
        if (status == Transaction::Status::DoneStatus) {
            updateAppState(installation, resource);
        }
    });
    return transaction;
}

Transaction* FlatpakBackend::installApplication(AbstractResource *app)
{
    return installApplication(app, {});
}

Transaction* FlatpakBackend::removeApplication(AbstractResource *app)
{
    FlatpakResource *resource = qobject_cast<FlatpakResource*>(app);

    if (resource->type() == FlatpakResource::Source) {
        // Let source backend handle this
        if (m_sources->removeSource(resource->flatpakName())) {
            resource->setState(AbstractResource::None);
        }
        return nullptr;
    }

    FlatpakInstallation *installation = resource->installation();
    FlatpakTransaction *transaction = new FlatpakTransaction(resource, Transaction::RemoveRole);

    connect(transaction, &FlatpakTransaction::statusChanged, [this, installation, resource] (Transaction::Status status) {
        if (status == Transaction::Status::DoneStatus) {
            updateAppSize(installation, resource);
        }
    });
    return transaction;
}

void FlatpakBackend::checkForUpdates()
{
    for (auto installation : qAsConst(m_installations)) {
        // Load local updates, comparing current and latest commit
        loadLocalUpdates(installation);

        // Load updates from remote repositories
        loadRemoteUpdates(installation);
    }
}

AbstractResource * FlatpakBackend::resourceForFile(const QUrl &url)
{
    if (!url.isLocalFile()) {
        return nullptr;
    }

    FlatpakResource *resource = nullptr;
    if (url.path().endsWith(QLatin1String(".flatpak"))) {
        resource = addAppFromFlatpakBundle(url);
    } else if (url.path().endsWith(QLatin1String(".flatpakref"))) {
        resource = addAppFromFlatpakRef(url);
    } else if (url.path().endsWith(QLatin1String(".flatpakrepo"))) {
        resource = addSourceFromFlatpakRepo(url);
    }

    return resource;
}

QString FlatpakBackend::displayName() const
{
    return QStringLiteral("Flatpak");
}

#include "FlatpakBackend.moc"
