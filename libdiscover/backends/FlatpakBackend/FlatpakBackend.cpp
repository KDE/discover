/*
 *   SPDX-FileCopyrightText: 2013 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *   SPDX-FileCopyrightText: 2017 Jan Grulich <jgrulich@redhat.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "FlatpakBackend.h"
#include "FlatpakFetchDataJob.h"
#include "FlatpakJobTransaction.h"
#include "FlatpakSourcesBackend.h"

#include <ReviewsBackend/Rating.h>
#include <Transaction/Transaction.h>
#include <appstream/AppStreamIntegration.h>
#include <appstream/AppStreamUtils.h>
#include <appstream/OdrsReviewsBackend.h>
#include <resources/SourcesModel.h>
#include <resources/StandardBackendUpdater.h>
#include <utils.h>

#include <AppStreamQt/bundle.h>
#include <AppStreamQt/icon.h>
#include <AppStreamQt/metadata.h>
#include <AppStreamQt/pool.h>

#include <KAboutData>
#include <KConfigGroup>
#include <KLocalizedString>
#include <KPluginFactory>
#include <KSharedConfig>

#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFutureWatcher>
#include <QNetworkAccessManager>
#include <QSettings>
#include <QTemporaryFile>
#include <QTextStream>
#include <QThread>
#include <QTimer>
#include <QtConcurrentRun>

#include <QRegularExpression>
#include <glib.h>

#include <sys/stat.h>

DISCOVER_BACKEND_PLUGIN(FlatpakBackend)

class FlatpakSource
{
public:
    FlatpakSource(FlatpakBackend *backend, FlatpakInstallation *installation)
        : m_remote(nullptr)
        , m_installation(installation)
        , m_backend(backend)
    {
        g_object_ref(m_installation);
    }

    FlatpakSource(FlatpakBackend *backend, FlatpakInstallation *installation, FlatpakRemote *remote)
        : m_remote(remote)
        , m_installation(installation)
        , m_backend(backend)
        , m_appstreamIconsDir(appstreamDir() + QLatin1String("/icons"))
    {
        g_object_ref(m_remote);
        g_object_ref(m_installation);
    }

    ~FlatpakSource()
    {
        if (m_remote) {
            g_object_unref(m_remote);
        }
        g_object_unref(m_installation);
    }

    QString url() const
    {
        return m_remote ? flatpak_remote_get_url(m_remote) : QString();
    }

    bool isEnabled() const
    {
        return m_remote && !flatpak_remote_get_disabled(m_remote);
    }

    QString appstreamIconsDir() const {
        return m_appstreamIconsDir;
    }
    QString appstreamDir() const {
        Q_ASSERT(m_remote);
        g_autoptr(GFile) appstreamDir = flatpak_remote_get_appstream_dir(m_remote, nullptr);
        if (!appstreamDir) {
            qWarning() << "No appstream dir for" << flatpak_remote_get_name(m_remote);
            return {};
        }
        g_autofree char *path_str = g_file_get_path(appstreamDir);
        return QString::fromUtf8(path_str);
    }

    QString name() const
    {
        return m_remote ? QString::fromUtf8(flatpak_remote_get_name(m_remote)) : QString();
    }

    FlatpakInstallation *installation() const
    {
        return m_installation;
    }

    void addResource(FlatpakResource *resource)
    {
        // Update app with all possible information we have
        if (!m_backend->parseMetadataFromAppBundle(resource)) {
            qWarning() << "Failed to parse metadata from app bundle for" << resource->name();
        }

        m_backend->updateAppState(resource);

        m_resources.insert(resource->uniqueId(), resource);
        if (!resource->extends().isEmpty()) {
            m_backend->m_extends.append(resource->extends());
            m_backend->m_extends.removeDuplicates();
        }

        QObject::connect(resource, &FlatpakResource::sizeChanged, m_backend, [this, resource] {
            if (!m_backend->isFetching())
                Q_EMIT m_backend->resourcesChanged(resource, {"size", "sizeDescription"});
        });
    }

    AppStream::Pool *m_pool = nullptr;
    QHash<FlatpakResource::Id, FlatpakResource *> m_resources;

private:
    FlatpakRemote *const m_remote;
    FlatpakInstallation *const m_installation;
    FlatpakBackend *const m_backend;
    const QString m_appstreamIconsDir;
};

QDebug operator<<(QDebug debug, const FlatpakResource::Id &id)
{
    QDebugStateSaver saver(debug);
    debug.nospace() << "FlatpakResource::Id(";
    debug.nospace() << "name:" << id.id << ',';
    debug.nospace() << "branch:" << id.branch << ',';
    debug.nospace() << "type:" << id.type;
    debug.nospace() << ')';
    return debug;
}

static FlatpakResource::Id idForRefString(const QStringView &ref)
{
    Q_ASSERT(!ref.isEmpty());

    auto parts = ref.split('/');
    // app/app.getspace.Space/x86_64/stable
    return {
        parts[0] == QLatin1String("app") ? FlatpakResource::DesktopApp : FlatpakResource::Runtime,
        parts[1].toString(),
        parts[3].toString(),
        parts[2].toString(),
    };
}

static FlatpakResource::Id idForInstalledRef(FlatpakInstalledRef *ref, const QString &postfix)
{
    const FlatpakResource::ResourceType appType = (flatpak_ref_get_kind(FLATPAK_REF(ref)) == FLATPAK_REF_KIND_APP //
                                                       ? FlatpakResource::DesktopApp
                                                       : FlatpakResource::Runtime);
    const QString appId = QLatin1String(flatpak_ref_get_name(FLATPAK_REF(ref))) + postfix;
    const QString arch = QString::fromUtf8(flatpak_ref_get_arch(FLATPAK_REF(ref)));
    const QString branch = QString::fromUtf8(flatpak_ref_get_branch(FLATPAK_REF(ref)));

    return {appType, appId, branch, arch};
}

FlatpakBackend::FlatpakBackend(QObject *parent)
    : AbstractResourcesBackend(parent)
    , m_updater(new StandardBackendUpdater(this))
    , m_reviews(AppStreamIntegration::global()->reviews())
    , m_refreshAppstreamMetadataJobs(0)
    , m_cancellable(g_cancellable_new())
    , m_threadPool(new QThreadPool(this))
{
    g_autoptr(GError) error = nullptr;

    connect(m_updater, &StandardBackendUpdater::updatesCountChanged, this, &FlatpakBackend::updatesCountChanged);

    // Load flatpak installation
    if (!setupFlatpakInstallations(&error)) {
        qWarning() << "Failed to setup flatpak installations:" << error->message;
    } else {
        loadAppsFromAppstreamData();

        m_sources = new FlatpakSourcesBackend(m_installations, this);
        SourcesModel::global()->addSourcesBackend(m_sources);
    }

    connect(m_reviews.data(), &OdrsReviewsBackend::ratingsReady, this, [this] {
        m_reviews->emitRatingFetched(this, kAppend<QList<AbstractResource *>>(m_flatpakSources, [](const auto &source) {
                                         return kTransform<QList<AbstractResource *>>(source->m_resources.values());
                                     }));
    });

    /* Override the umask to 022 to make it possible to share files between
     * the plasma-discover process and flatpak system helper process.
     *
     * See https://github.com/flatpak/flatpak/pull/2856/
     */
    umask(022);
}

FlatpakBackend::~FlatpakBackend()
{
    g_cancellable_cancel(m_cancellable);
    for (auto inst : qAsConst(m_installations))
        g_object_unref(inst);
    if (!m_threadPool.waitForDone(200)) {
        qDebug() << "could not kill them all" << m_threadPool.activeThreadCount();
    }
    m_threadPool.clear();

    g_object_unref(m_cancellable);
}

bool FlatpakBackend::isValid() const
{
    return m_sources && !m_installations.isEmpty();
}

class FlatpakFetchRemoteResourceJob : public QNetworkAccessManager
{
    Q_OBJECT
public:
    FlatpakFetchRemoteResourceJob(const QUrl &url, ResultsStream *stream, FlatpakBackend *backend)
        : QNetworkAccessManager(backend)
        , m_backend(backend)
        , m_stream(stream)
        , m_url(url)
    {
        connect(stream, &ResultsStream::destroyed, this, &QObject::deleteLater);
    }

    void start()
    {
        if (m_url.isLocalFile()) {
            QTimer::singleShot(0, m_stream, [this] {
                processFile(m_url);
            });
            return;
        }

        QNetworkRequest req(m_url);
        req.setAttribute(QNetworkRequest::FollowRedirectsAttribute, true);
        auto replyGet = get(req);
        connect(replyGet, &QNetworkReply::finished, this, [this, replyGet] {
            QScopedPointer<QNetworkReply, QScopedPointerDeleteLater> replyPtr(replyGet);
            if (replyGet->error() != QNetworkReply::NoError) {
                qWarning() << "couldn't download" << m_url << replyGet->errorString();
                m_stream->finish();
                return;
            }
            const QUrl fileUrl = QUrl::fromLocalFile(QStandardPaths::writableLocation(QStandardPaths::TempLocation) //
                                                     + QLatin1Char('/') + m_url.fileName());
            auto replyPut = put(QNetworkRequest(fileUrl), replyGet->readAll());
            connect(replyPut, &QNetworkReply::finished, this, [this, fileUrl, replyPut]() {
                QScopedPointer<QNetworkReply, QScopedPointerDeleteLater> replyPtr(replyPut);
                if (replyPut->error() != QNetworkReply::NoError) {
                    qWarning() << "couldn't save" << m_url << replyPut->errorString();
                    m_stream->finish();
                    return;
                }
                if (!fileUrl.isLocalFile()) {
                    m_stream->finish();
                    return;
                }

                processFile(fileUrl);
            });
        });
    }

    void processFile(const QUrl &fileUrl)
    {
        const auto path = fileUrl.toLocalFile();
        if (path.endsWith(QLatin1String(".flatpak"))) {
            m_backend->addAppFromFlatpakBundle(fileUrl, m_stream);
        } else if (path.endsWith(QLatin1String(".flatpakref"))) {
            m_backend->addAppFromFlatpakRef(fileUrl, m_stream);
        } else if (path.endsWith(QLatin1String(".flatpakrepo"))) {
            m_backend->addSourceFromFlatpakRepo(fileUrl, m_stream);
        } else {
            qWarning() << "unrecognized format" << fileUrl;
        }
    }

private:
    FlatpakBackend *const m_backend;
    ResultsStream *const m_stream;
    const QUrl m_url;
};

FlatpakRemote *FlatpakBackend::getFlatpakRemoteByUrl(const QString &url, FlatpakInstallation *installation) const
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

FlatpakInstalledRef *FlatpakBackend::getInstalledRefForApp(FlatpakResource *resource) const
{
    g_autoptr(GError) localError = nullptr;

    const auto type = resource->resourceType() == FlatpakResource::DesktopApp ? FLATPAK_REF_KIND_APP : FLATPAK_REF_KIND_RUNTIME;

    FlatpakInstalledRef *ref = flatpak_installation_get_installed_ref(resource->installation(),
                                                                      type,
                                                                      resource->flatpakName().toUtf8().constData(),
                                                                      resource->arch().toUtf8().constData(),
                                                                      resource->branch().toUtf8().constData(),
                                                                      m_cancellable,
                                                                      &localError);
    return ref;
}

QString refToBundleId(FlatpakRef *ref)
{
    return QString(flatpak_ref_get_kind(ref) == FLATPAK_REF_KIND_APP ? "app/" : "runtime/") + flatpak_ref_get_name(ref) + '/' + flatpak_ref_get_arch(ref) + '/'
        + flatpak_ref_get_branch(ref);
}

FlatpakResource *FlatpakBackend::getAppForInstalledRef(FlatpakInstallation *installation, FlatpakInstalledRef *ref) const
{
    auto id = idForInstalledRef(ref, {});
    for (const auto &source : m_flatpakSources) {
        auto ret = source->m_resources.value(id);
        if (ret) {
            return ret;
        }
    }
    auto id2 = idForInstalledRef(ref, QStringLiteral(".desktop"));
    for (const auto &source : m_flatpakSources) {
        auto ret = source->m_resources.value(id2);
        if (ret) {
            return ret;
        }
    }

    const QLatin1String name(flatpak_ref_get_name(FLATPAK_REF(ref)));

    const QString origin = QString::fromUtf8(flatpak_installed_ref_get_origin(ref));
    auto source = findSource(installation, origin);
    const QString pathExports = FlatpakResource::installationPath(installation) + QLatin1String("/exports/");
    const QString pathApps = pathExports + QLatin1String("share/applications/");
    AppStream::Component cid;
    if (source && source->m_pool) {
        QList<AppStream::Component> comps = source->m_pool->componentsById(name);
        if (comps.isEmpty()) {
            comps = source->m_pool->componentsById(name + ".desktop");
        }

        if (comps.isEmpty()) {
            const QString bundleId = refToBundleId(FLATPAK_REF(ref));
            comps = kFilter<QList<AppStream::Component>>(comps, [&bundleId](const AppStream::Component &comp) -> bool {
                return comp.bundle(AppStream::Bundle::Kind::KindFlatpak).id() == bundleId;
            });
        }

        if (comps.count() >= 1) {
            Q_ASSERT(comps.count() == 1);
            cid = comps.constFirst();
        }
    }

    if (!cid.isValid()) {
        AppStream::Metadata metadata;
        const QString fnDesktop = pathApps + name + QLatin1String(".desktop");
        AppStream::Metadata::MetadataError error = metadata.parseFile(fnDesktop, AppStream::Metadata::FormatKindDesktopEntry);
        if (error != AppStream::Metadata::MetadataErrorNoError) {
            if (QFile::exists(fnDesktop))
                qDebug() << "Failed to parse appstream metadata:" << error << fnDesktop;

            cid.setId(QString::fromLatin1(flatpak_ref_get_name(FLATPAK_REF(ref))));
#if FLATPAK_CHECK_VERSION(1, 1, 2)
            cid.setName(QString::fromUtf8(flatpak_installed_ref_get_appdata_name(ref)));
#endif
        } else
            cid = metadata.component();
    }

    FlatpakResource *resource = new FlatpakResource(cid, source->installation(), const_cast<FlatpakBackend *>(this));
    resource->setOrigin(source->name());
    resource->setIconPath(pathExports);
    resource->updateFromRef(FLATPAK_REF(ref));
    resource->setState(AbstractResource::Installed);
    source->addResource(resource);
    return resource;
}

QSharedPointer<FlatpakSource> FlatpakBackend::findSource(FlatpakInstallation *installation, const QString &origin) const
{
    for (const auto &source : m_flatpakSources) {
        if (source->installation() == installation && source->name() == origin) {
            return source;
        }
    }

    Q_UNREACHABLE();
}

FlatpakResource *FlatpakBackend::getRuntimeForApp(FlatpakResource *resource) const
{
    FlatpakResource *runtime = nullptr;
    const QString runtimeName = resource->runtime();
    const auto runtimeInfo = runtimeName.splitRef(QLatin1Char('/'));

    if (runtimeInfo.count() != 3) {
        return runtime;
    }

    for (const auto &source : m_flatpakSources) {
        for (auto it = source->m_resources.constBegin(), itEnd = source->m_resources.constEnd(); it != itEnd; ++it) {
            const auto &id = it.key();
            if (id.type == FlatpakResource::Runtime && id.id == runtimeInfo.at(0) && id.branch == runtimeInfo.at(2)) {
                runtime = *it;
                break;
            }
        }
    }

    // TODO if runtime wasn't found, create a new one from available info
    if (!runtime) {
        qWarning() << "could not find runtime" << runtimeName << resource;
    }

    return runtime;
}

void FlatpakBackend::addAppFromFlatpakBundle(const QUrl &url, ResultsStream *stream)
{
    auto x = qScopeGuard([stream] {
        stream->finish();
    });
    g_autoptr(GBytes) appstreamGz = nullptr;
    g_autoptr(GError) localError = nullptr;
    g_autoptr(GFile) file = nullptr;
    g_autoptr(FlatpakBundleRef) bundleRef = nullptr;
    AppStream::Component asComponent;

    file = g_file_new_for_path(url.toLocalFile().toUtf8().constData());
    bundleRef = flatpak_bundle_ref_new(file, &localError);

    if (!bundleRef) {
        qWarning() << "Failed to load bundle:" << localError->message;
        return;
    }

    gsize len = 0;
    g_autoptr(GBytes) metadata = flatpak_bundle_ref_get_metadata(bundleRef);
    const QByteArray metadataContent((char *)g_bytes_get_data(metadata, &len));

    appstreamGz = flatpak_bundle_ref_get_appstream(bundleRef);
    if (appstreamGz) {
        g_autoptr(GZlibDecompressor) decompressor = nullptr;
        g_autoptr(GInputStream) streamGz = nullptr;
        g_autoptr(GInputStream) streamData = nullptr;
        g_autoptr(GBytes) appstream = nullptr;

        /* decompress data */
        decompressor = g_zlib_decompressor_new(G_ZLIB_COMPRESSOR_FORMAT_GZIP);
        streamGz = g_memory_input_stream_new_from_bytes(appstreamGz);
        if (!streamGz) {
            return;
        }

        streamData = g_converter_input_stream_new(streamGz, G_CONVERTER(decompressor));

        appstream = g_input_stream_read_bytes(streamData, 0x100000, m_cancellable, &localError);
        if (!appstream) {
            qWarning() << "Failed to extract appstream metadata from bundle:" << localError->message;
            return;
        }

        gsize len = 0;
        gconstpointer data = g_bytes_get_data(appstream, &len);

        AppStream::Metadata metadata;
        metadata.setFormatStyle(AppStream::Metadata::FormatStyleCollection);
        AppStream::Metadata::MetadataError error = metadata.parse(QString::fromUtf8((char *)data, len), AppStream::Metadata::FormatKindXml);
        if (error != AppStream::Metadata::MetadataErrorNoError) {
            qWarning() << "Failed to parse appstream metadata: " << error;
            return;
        }

        const QList<AppStream::Component> components = metadata.components();
        if (components.size()) {
            asComponent = components.first();
        } else {
            qWarning() << "Failed to parse appstream metadata";
            return;
        }
    } else {
        qWarning() << "No appstream metadata in bundle";

        QTemporaryFile tempFile;
        tempFile.setAutoRemove(false);
        if (!tempFile.open()) {
            qWarning() << "Failed to get metadata file";
            return;
        }

        tempFile.write(metadataContent);
        tempFile.close();

        // Parse the temporary file
        QSettings setting(tempFile.fileName(), QSettings::NativeFormat);
        setting.beginGroup(QLatin1String("Application"));

        asComponent.setName(setting.value(QLatin1String("name")).toString());

        tempFile.remove();
    }

    FlatpakResource *resource = new FlatpakResource(asComponent, preferredInstallation(), this);
    if (!updateAppMetadata(resource, metadataContent)) {
        delete resource;
        qWarning() << "Failed to update metadata from app bundle";
        return;
    }

    g_autoptr(GBytes) iconData = flatpak_bundle_ref_get_icon(bundleRef, 128);
    if (!iconData) {
        iconData = flatpak_bundle_ref_get_icon(bundleRef, 64);
    }

    if (iconData) {
        gsize len = 0;
        char *data = (char *)g_bytes_get_data(iconData, &len);

        QPixmap pixmap;
        pixmap.loadFromData(QByteArray(data, len), "PNG");
        resource->setBundledIcon(pixmap);
    }

    const QString origin = QString::fromUtf8(flatpak_bundle_ref_get_origin(bundleRef));

    resource->setDownloadSize(0);
    resource->setInstalledSize(flatpak_bundle_ref_get_installed_size(bundleRef));
    resource->setPropertyState(FlatpakResource::DownloadSize, FlatpakResource::AlreadyKnown);
    resource->setPropertyState(FlatpakResource::InstalledSize, FlatpakResource::AlreadyKnown);
    resource->setFlatpakFileType(FlatpakResource::FileFlatpak);
    resource->setOrigin(origin.isEmpty() ? i18n("Local bundle") : origin);
    resource->setResourceFile(url);
    resource->setState(FlatpakResource::None);
    resource->setType(FlatpakResource::DesktopApp);

    if (!m_localSource) {
        m_localSource.reset(new FlatpakSource(this, preferredInstallation()));
        m_flatpakSources += m_localSource;
    }
    m_localSource->addResource(resource);
    stream->resourcesFound({resource});
}

void FlatpakBackend::addAppFromFlatpakRef(const QUrl &url, ResultsStream *stream)
{
    QSettings settings(url.toLocalFile(), QSettings::NativeFormat);
    const QString refurl = settings.value(QStringLiteral("Flatpak Ref/Url")).toString();
    const QString name = settings.value(QStringLiteral("Flatpak Ref/Name")).toString();
    const QString remoteName = settings.value(QStringLiteral("Flatpak Ref/SuggestRemoteName")).toString();
    g_autoptr(GError) error = nullptr;

    // If we already added the remote, just go with it
    g_autoptr(FlatpakRemote) remote = flatpak_installation_get_remote_by_name(preferredInstallation(), remoteName.toUtf8().constData(), m_cancellable, &error);
    if (remote && flatpak_remote_get_url(remote) != refurl) {
        remote = nullptr;
    }
    if (remote) {
        m_refreshAppstreamMetadataJobs++;
        auto source = integrateRemote(preferredInstallation(), remote);
        if (source) {
            auto searchComponent = [this, stream, source, name] {
                const auto comps = source->m_pool->componentsById(name);
                auto resources = kTransform<QVector<AbstractResource *>>(comps, [this, source](const auto &comp) {
                    return resourceForComponent(comp, source);
                });
                stream->resourcesFound(resources);
                stream->finish();
            };
            if (source->m_pool) {
                QTimer::singleShot(0, this, searchComponent);
            } else {
                connect(this, &FlatpakBackend::initialized, stream, searchComponent);
            }
            return;
        }
    }

    g_autoptr(FlatpakRemoteRef) remoteRef = nullptr;
    {
        QFile f(url.toLocalFile());
        if (!f.open(QFile::ReadOnly | QFile::Text)) {
            stream->finish();
            return;
        }

        QByteArray contents = f.readAll();

        g_autoptr(GBytes) bytes = g_bytes_new(contents.data(), contents.size());

        remoteRef = flatpak_installation_install_ref_file(preferredInstallation(), bytes, m_cancellable, &error);
        if (!remoteRef) {
            qWarning() << "Failed to create install ref file:" << error->message;
            AbstractResourcesBackend::Filters filter;
            filter.resourceUrl = QUrl(QLatin1String("appstream://") + name);
            auto streamKnown = search(filter);
            connect(streamKnown, &ResultsStream::resourcesFound, stream, &ResultsStream::resourcesFound);
            connect(streamKnown, &ResultsStream::destroyed, stream, &ResultsStream::finish);
            return;
        }
    }

    auto ref = FLATPAK_REF(remoteRef);

    AppStream::Component asComponent;
    asComponent.addUrl(AppStream::Component::UrlKindHomepage, settings.value(QStringLiteral("Flatpak Ref/Homepage")).toString());
    asComponent.setDescription(settings.value(QStringLiteral("Flatpak Ref/Description")).toString());
    asComponent.setName(settings.value(QStringLiteral("Flatpak Ref/Title")).toString());
    asComponent.setSummary(settings.value(QStringLiteral("Flatpak Ref/Comment")).toString());
    asComponent.setId(name);

    AppStream::Bundle b;
    b.setKind(AppStream::Bundle::KindFlatpak);
    b.setId(flatpak_ref_format_ref(ref));
    asComponent.addBundle(b);

    const QString iconUrl = settings.value(QStringLiteral("Flatpak Ref/Icon")).toString();
    if (!iconUrl.isEmpty()) {
        AppStream::Icon icon;
        icon.setKind(AppStream::Icon::KindRemote);
        icon.setUrl(QUrl(iconUrl));
        asComponent.addIcon(icon);
    }

    auto resource = new FlatpakResource(asComponent, preferredInstallation(), this);
    resource->setFlatpakFileType(FlatpakResource::FileFlatpakRef);
    resource->setOrigin(remoteName);
    resource->updateFromRef(ref);

    QUrl runtimeUrl = QUrl(settings.value(QStringLiteral("Flatpak Ref/RuntimeRepo")).toString());
    auto refSource = QSharedPointer<FlatpakSource>::create(this, preferredInstallation());
    m_flatpakSources += refSource;
    if (!runtimeUrl.isEmpty()) {
        // We need to fetch metadata to find information about required runtime
        auto fw = new QFutureWatcher<QByteArray>(this);
        connect(fw, &QFutureWatcher<QByteArray>::finished, this, [this, resource, fw, runtimeUrl, stream, refSource]() {
            fw->deleteLater();
            const auto metadata = fw->result();
            // Even when we failed to fetch information about runtime we still want to show the application
            if (metadata.isEmpty()) {
                Q_EMIT onFetchMetadataFinished(resource, metadata);
            } else {
                updateAppMetadata(resource, metadata);

                auto runtime = getRuntimeForApp(resource);
                if (!runtime || (runtime && !runtime->isInstalled())) {
                    auto repoStream = new ResultsStream(QLatin1String("FlatpakStream-searchrepo-") + runtimeUrl.toString());
                    connect(repoStream,
                            &ResultsStream::resourcesFound,
                            this,
                            [this, resource, stream, refSource](const QVector<AbstractResource *> &resources) {
                                for (auto res : resources) {
                                    installApplication(res);
                                }
                                refSource->addResource(resource);
                                stream->resourcesFound({resource});
                                stream->finish();
                            });

                    auto fetchRemoteResource = new FlatpakFetchRemoteResourceJob(runtimeUrl, repoStream, this);
                    fetchRemoteResource->start();
                    return;
                } else {
                    refSource->addResource(resource);
                }
            }
            stream->resourcesFound({resource});
            stream->finish();
        });
        fw->setFuture(QtConcurrent::run(&m_threadPool, &FlatpakRunnables::fetchMetadata, resource, m_cancellable));
    } else {
        refSource->addResource(resource);
        stream->resourcesFound({resource});
        stream->finish();
    }

}

void FlatpakBackend::addSourceFromFlatpakRepo(const QUrl &url, ResultsStream *stream)
{
    auto x = qScopeGuard([stream] {
        stream->finish();
    });
    Q_ASSERT(url.isLocalFile());
    QSettings settings(url.toLocalFile(), QSettings::NativeFormat);

    const QString gpgKey = settings.value(QStringLiteral("Flatpak Repo/GPGKey")).toString();
    const QString title = settings.value(QStringLiteral("Flatpak Repo/Title")).toString();
    const QString repoUrl = settings.value(QStringLiteral("Flatpak Repo/Url")).toString();

    if (gpgKey.isEmpty() || title.isEmpty() || repoUrl.isEmpty()) {
        return;
    }

    if (gpgKey.startsWith(QLatin1String("http://")) || gpgKey.startsWith(QLatin1String("https://"))) {
        return;
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

    g_autoptr(FlatpakRemote) repo =
        flatpak_installation_get_remote_by_name(preferredInstallation(), resource->flatpakName().toUtf8().constData(), m_cancellable, nullptr);
    if (!repo) {
        resource->setState(AbstractResource::State::None);
    } else {
        resource->setState(AbstractResource::State::Installed);
    }

    stream->resourcesFound({resource});
}

void FlatpakBackend::loadAppsFromAppstreamData()
{
    for (auto installation : qAsConst(m_installations)) {
        // Load applications from appstream metadata
        if (g_cancellable_is_cancelled(m_cancellable))
            break;

        if (!loadAppsFromAppstreamData(installation)) {
            qWarning() << "Failed to load packages from appstream data from installation" << installation;
        }
    }
}

bool FlatpakBackend::loadAppsFromAppstreamData(FlatpakInstallation *flatpakInstallation)
{
    Q_ASSERT(flatpakInstallation);

    g_autoptr(GPtrArray) remotes = flatpak_installation_list_remotes(flatpakInstallation, m_cancellable, nullptr);
    if (!remotes) {
        return false;
    }

    m_refreshAppstreamMetadataJobs += remotes->len;

    for (uint i = 0; i < remotes->len; i++) {
        FlatpakRemote *remote = FLATPAK_REMOTE(g_ptr_array_index(remotes, i));
        g_autoptr(GFile) fileTimestamp = flatpak_remote_get_appstream_timestamp(remote, flatpak_get_default_arch());

        g_autofree char *path_str = g_file_get_path(fileTimestamp);
        QFileInfo fileInfo(QFile::encodeName(path_str));
        // Refresh appstream metadata in case they have never been refreshed or the cache is older than 6 hours
        if (!fileInfo.exists() || fileInfo.lastModified().toUTC().secsTo(QDateTime::currentDateTimeUtc()) > 21600) {
            refreshAppstreamMetadata(flatpakInstallation, remote);
        } else {
            integrateRemote(flatpakInstallation, remote);
        }
    }
    return true;
}

void FlatpakBackend::metadataRefreshed()
{
    m_refreshAppstreamMetadataJobs--;
    if (m_refreshAppstreamMetadataJobs == 0) {
        checkForUpdates();
    }
}

QSharedPointer<FlatpakSource> FlatpakBackend::integrateRemote(FlatpakInstallation *flatpakInstallation, FlatpakRemote *remote)
{
    Q_ASSERT(m_refreshAppstreamMetadataJobs != 0);
    for (auto source : qAsConst(m_flatpakSources)) {
        if (source->url() == flatpak_remote_get_url(remote)) {
            qDebug() << "do not add a source twice" << source << remote;
            metadataRefreshed();
            return source;
        }
    }
    for (auto source : qAsConst(m_flatpakLoadingSources)) {
        if (source->url() == flatpak_remote_get_url(remote)) {
            qDebug() << "do not add a source twice" << source << remote;
            metadataRefreshed();
            return source;
        }
    }

    auto source = QSharedPointer<FlatpakSource>::create(this, flatpakInstallation, remote);
    if (!source->isEnabled() || flatpak_remote_get_noenumerate(remote)) {
        metadataRefreshed();
        return {};
    }

    const QString appstreamDirPath = source->appstreamDir();
    if (!QFile::exists(appstreamDirPath)) {
        qWarning() << "No" << appstreamDirPath << "appstream metadata found for" << source->name();
        metadataRefreshed();
        return {};
    }

    AppStream::Pool *pool = new AppStream::Pool(this);
    auto fw = new QFutureWatcher<bool>(this);
    const auto sourceName = source->name();
    connect(fw, &QFutureWatcher<bool>::finished, this, [this, fw, pool, source]() {
        source->m_pool = pool;
        m_flatpakLoadingSources.removeAll(source);
        if (fw->result()) {
            m_flatpakSources += source;
        } else {
            qWarning() << "Could not open the AppStream metadata pool" << pool->lastError();
        }
        metadataRefreshed();
        acquireFetching(false);
        fw->deleteLater();
    });
    acquireFetching(true);
    pool->clearMetadataLocations();
    pool->addMetadataLocation(appstreamDirPath);
    pool->setFlags(AppStream::Pool::FlagReadCollection);
    pool->setCacheFlags(AppStream::Pool::CacheFlagUseUser);

    const QString subdir = flatpak_installation_get_id(flatpakInstallation) + QLatin1Char('/') + sourceName;
    pool->setCacheLocation(QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + "/flatpak-appstream/" + subdir);
    QDir().mkpath(pool->cacheLocation());
    fw->setFuture(QtConcurrent::run(&m_threadPool, pool, &AppStream::Pool::load));
    m_flatpakLoadingSources << source;
    return source;
}

void FlatpakBackend::loadLocalUpdates(FlatpakInstallation *flatpakInstallation)
{
    g_autoptr(GError) localError = nullptr;
    g_autoptr(GPtrArray) refs = flatpak_installation_list_installed_refs(flatpakInstallation, m_cancellable, &localError);
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
            updateAppSize(resource);
        }
    }
}

bool FlatpakBackend::parseMetadataFromAppBundle(FlatpakResource *resource)
{
    g_autoptr(GError) localError = nullptr;
    g_autoptr(FlatpakRef) ref = flatpak_ref_parse(resource->ref().toUtf8().constData(), &localError);
    if (!ref) {
        qWarning() << "Failed to parse" << resource->ref() << localError->message;
        return false;
    } else {
        resource->updateFromRef(ref);
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
        g_object_ref(m_remote);
        connect(this, &FlatpakRefreshAppstreamMetadataJob::finished, this, &QObject::deleteLater);
    }

    ~FlatpakRefreshAppstreamMetadataJob()
    {
        g_object_unref(m_remote);
        g_object_unref(m_cancellable);
    }

    void cancel()
    {
        g_cancellable_cancel(m_cancellable);
    }

    void run() override
    {
        g_autoptr(GError) localError = nullptr;

#if FLATPAK_CHECK_VERSION(0, 9, 4)
        // With Flatpak 0.9.4 we can use flatpak_installation_update_appstream_full_sync() providing progress reporting which we don't use at this moment, but
        // still better to use newer function in case the previous one gets deprecated
        if (!flatpak_installation_update_appstream_full_sync(m_installation,
                                                             flatpak_remote_get_name(m_remote),
                                                             nullptr,
                                                             nullptr,
                                                             nullptr,
                                                             nullptr,
                                                             m_cancellable,
                                                             &localError)) {
#else
        if (!flatpak_installation_update_appstream_sync(m_installation, flatpak_remote_get_name(m_remote), nullptr, nullptr, m_cancellable, &localError)) {
#endif
            const QString error = localError ? QString::fromUtf8(localError->message) : QStringLiteral("<no error>");
            qWarning() << "Failed to refresh appstream metadata for " << flatpak_remote_get_name(m_remote) << ": " << error;
            Q_EMIT jobRefreshAppstreamMetadataFailed(error);
        } else {
            Q_EMIT jobRefreshAppstreamMetadataFinished(m_installation, m_remote);
        }
    }

Q_SIGNALS:
    void jobRefreshAppstreamMetadataFailed(const QString &errorMessage);
    void jobRefreshAppstreamMetadataFinished(FlatpakInstallation *installation, FlatpakRemote *remote);

private:
    GCancellable *m_cancellable;
    FlatpakInstallation *m_installation;
    FlatpakRemote *m_remote;
};

void FlatpakBackend::refreshAppstreamMetadata(FlatpakInstallation *installation, FlatpakRemote *remote)
{
    FlatpakRefreshAppstreamMetadataJob *job = new FlatpakRefreshAppstreamMetadataJob(installation, remote);
    connect(job, &FlatpakRefreshAppstreamMetadataJob::jobRefreshAppstreamMetadataFailed, this, &FlatpakBackend::metadataRefreshed);
    connect(job, &FlatpakRefreshAppstreamMetadataJob::jobRefreshAppstreamMetadataFailed, this, [this](const QString &errorMessage) {
        Q_EMIT passiveMessage(errorMessage);
    });
    connect(job, &FlatpakRefreshAppstreamMetadataJob::jobRefreshAppstreamMetadataFinished, this, &FlatpakBackend::integrateRemote);
    connect(job, &FlatpakRefreshAppstreamMetadataJob::finished, this, [this] {
        acquireFetching(false);
    });

    acquireFetching(true);
    job->start();
}

bool FlatpakBackend::setupFlatpakInstallations(GError **error)
{
    if (qEnvironmentVariableIsSet("FLATPAK_TEST_MODE")) {
        const QString path = QStandardPaths::writableLocation(QStandardPaths::TempLocation) + QLatin1String("/discover-flatpak-test");
        qDebug() << "running flatpak backend on test mode" << path;
        g_autoptr(GFile) file = g_file_new_for_path(QFile::encodeName(path).constData());
        m_installations << flatpak_installation_new_for_path(file, true, m_cancellable, error);
        return true;
    }

    g_autoptr(GPtrArray) installations = flatpak_get_system_installations(m_cancellable, error);
    if (*error) {
        qWarning() << "Failed to call flatpak_get_system_installations:" << (*error)->message;
    }
    for (uint i = 0; installations && i < installations->len; i++) {
        auto installation = FLATPAK_INSTALLATION(g_ptr_array_index(installations, i));
        g_object_ref(installation);
        m_installations << installation;
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
    if (resource->state() < AbstractResource::Installed)
        resource->setState(AbstractResource::Installed);
}

bool FlatpakBackend::updateAppMetadata(FlatpakResource *resource)
{
    if (resource->resourceType() != FlatpakResource::DesktopApp) {
        return true;
    }

    const QString path = resource->installPath() + QStringLiteral("/metadata");

    if (QFile::exists(path)) {
        return updateAppMetadata(resource, path);
    } else {
        auto fw = new QFutureWatcher<QByteArray>(this);
        connect(fw, &QFutureWatcher<QByteArray>::finished, this, [this, resource, fw]() {
            const auto metadata = fw->result();
            if (!metadata.isEmpty())
                onFetchMetadataFinished(resource, metadata);
            fw->deleteLater();
        });
        fw->setFuture(QtConcurrent::run(&m_threadPool, &FlatpakRunnables::fetchMetadata, resource, m_cancellable));

        // Return false to indicate we cannot continue (right now used only in updateAppSize())
        return false;
    }
}

void FlatpakBackend::onFetchMetadataFinished(FlatpakResource *resource, const QByteArray &metadata)
{
    updateAppMetadata(resource, metadata);

    // Right now we attempt to update metadata for calculating the size so call updateSizeFromRemote()
    // as it's what we want. In future if there are other reason to update metadata we will need to somehow
    // distinguish between these calls
    updateAppSizeFromRemote(resource);
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
    // We just find the runtime with a regex, QSettings only can read from disk (and so does KConfig)
    const QRegularExpression rx(QStringLiteral("runtime=(.*)"));
    const auto match = rx.match(QString::fromUtf8(data));
    if (!match.isValid()) {
        return false;
    }

    resource->setRuntime(match.captured(1));
    return true;
}

bool FlatpakBackend::updateAppSize(FlatpakResource *resource)
{
    // Check if the size is already set, we should also distinguish between download and installed size,
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
        if (!updateAppMetadata(resource)) {
            return false;
        }
    }

    return updateAppSizeFromRemote(resource);
}

bool FlatpakBackend::updateAppSizeFromRemote(FlatpakResource *resource)
{
    // Calculate the runtime size
    if (resource->state() == AbstractResource::None && resource->resourceType() == FlatpakResource::DesktopApp) {
        auto runtime = getRuntimeForApp(resource);
        if (runtime) {
            // Re-check runtime state if case a new one was created
            updateAppState(runtime);

            if (!runtime->isInstalled()) {
                if (!updateAppSize(runtime)) {
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
        ref = getInstalledRefForApp(resource);
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

        if (resource->propertyState(FlatpakResource::DownloadSize) == FlatpakResource::Fetching) {
            return true;
        }

        auto futureWatcher = new QFutureWatcher<FlatpakRemoteRef *>(this);
        connect(futureWatcher, &QFutureWatcher<FlatpakRemoteRef *>::finished, this, [this, resource, futureWatcher]() {
            g_autoptr(FlatpakRemoteRef) remoteRef = futureWatcher->result();
            if (remoteRef) {
                onFetchSizeFinished(resource, flatpak_remote_ref_get_download_size(remoteRef), flatpak_remote_ref_get_installed_size(remoteRef));
            } else {
                resource->setPropertyState(FlatpakResource::DownloadSize, FlatpakResource::UnknownOrFailed);
                resource->setPropertyState(FlatpakResource::InstalledSize, FlatpakResource::UnknownOrFailed);
            }
            futureWatcher->deleteLater();
        });
        resource->setPropertyState(FlatpakResource::DownloadSize, FlatpakResource::Fetching);
        resource->setPropertyState(FlatpakResource::InstalledSize, FlatpakResource::Fetching);

        futureWatcher->setFuture(QtConcurrent::run(&m_threadPool, &FlatpakRunnables::findRemoteRef, resource, m_cancellable));
    }

    return true;
}

void FlatpakBackend::onFetchSizeFinished(FlatpakResource *resource, guint64 downloadSize, guint64 installedSize)
{
    FlatpakResource *runtime = nullptr;
    if (resource->state() == AbstractResource::None && resource->resourceType() == FlatpakResource::DesktopApp) {
        runtime = getRuntimeForApp(resource);
    }

    if (runtime && !runtime->isInstalled()) {
        resource->setDownloadSize(runtime->downloadSize() + downloadSize);
    } else {
        resource->setDownloadSize(downloadSize);
    }
    resource->setInstalledSize(installedSize);
}

void FlatpakBackend::updateAppState(FlatpakResource *resource)
{
    g_autoptr(FlatpakInstalledRef) ref = getInstalledRefForApp(resource);
    if (ref) {
        // If the app is installed, we can set information about commit, arch etc.
        updateAppInstalledMetadata(ref, resource);
    } else {
        // TODO check if the app is actually still available
        resource->setState(AbstractResource::None);
    }
}

void FlatpakBackend::acquireFetching(bool f)
{
    if (f)
        m_isFetching++;
    else
        m_isFetching--;

    if ((!f && m_isFetching == 0) || (f && m_isFetching == 1)) {
        Q_EMIT fetchingChanged();
    }

    if (m_isFetching == 0)
        Q_EMIT initialized();
}

int FlatpakBackend::updatesCount() const
{
    return m_updater->updatesCount();
}

bool FlatpakBackend::flatpakResourceLessThan(AbstractResource *l, AbstractResource *r) const
{
    // clang-format off
    return (l->isInstalled() != r->isInstalled()) ? l->isInstalled()
         : (l->origin() != r->origin()) ? m_sources->originIndex(l->origin()) < m_sources->originIndex(r->origin())
         : (l->rating() && r->rating() && l->rating()->ratingPoints() != r->rating()->ratingPoints()) ? l->rating()->ratingPoints() > r->rating()->ratingPoints()
         : l < r;
    // clang-format on
}

ResultsStream *FlatpakBackend::search(const AbstractResourcesBackend::Filters &filter)
{
    const auto fileName = filter.resourceUrl.fileName();
    if (fileName.endsWith(QLatin1String(".flatpakrepo")) || fileName.endsWith(QLatin1String(".flatpakref")) || fileName.endsWith(QLatin1String(".flatpak"))) {
        auto stream = new ResultsStream(QLatin1String("FlatpakStream-http-") + fileName);
        FlatpakFetchRemoteResourceJob *fetchResourceJob = new FlatpakFetchRemoteResourceJob(filter.resourceUrl, stream, this);
        fetchResourceJob->start();
        return stream;
    } else if (filter.resourceUrl.scheme() == QLatin1String("appstream")) {
        return findResourceByPackageName(filter.resourceUrl);
    } else if (!filter.resourceUrl.isEmpty() || (!filter.extends.isEmpty() && !m_extends.contains(filter.extends)))
        return new ResultsStream(QStringLiteral("FlatpakStream-void"), {});
    else if (filter.state == AbstractResource::Upgradeable) {
        auto stream = new ResultsStream(QStringLiteral("FlatpakStream-upgradeable"));
        auto f = [this, stream] {
            QVector<AbstractResource *> resources;
            for (auto installation : m_installations) {
                g_autoptr(GError) localError = nullptr;
                g_autoptr(GPtrArray) refs = flatpak_installation_list_installed_refs_for_update(installation, m_cancellable, &localError);
                if (!refs) {
                    qWarning() << "Failed to get list of installed refs for listing updates:" << localError->message;
                    continue;
                }

                resources.reserve(resources.size() + refs->len);
                for (uint i = 0; i < refs->len; i++) {
                    FlatpakInstalledRef *ref = FLATPAK_INSTALLED_REF(g_ptr_array_index(refs, i));
                    auto resource = getAppForInstalledRef(installation, ref);
                    resource->setState(AbstractResource::Upgradeable);
                    updateAppSize(resource);
                    if (resource->resourceType() == FlatpakResource::Runtime) {
                        resources.prepend(resource);
                    } else {
                        resources.append(resource);
                    }
                }
            }
            if (!resources.isEmpty())
                Q_EMIT stream->resourcesFound(resources);
            stream->finish();
        };

        if (isFetching()) {
            connect(this, &FlatpakBackend::initialized, stream, f);
        } else {
            QTimer::singleShot(0, this, f);
        }
        return stream;
    } else if (filter.state == AbstractResource::Installed) {
        auto stream = new ResultsStream(QStringLiteral("FlatpakStream-installed"));
        auto f = [this, stream] {
            QVector<AbstractResource *> resources;
            for (auto installation : m_installations) {
                g_autoptr(GError) localError = nullptr;
                g_autoptr(GPtrArray) refs = flatpak_installation_list_installed_refs(installation, m_cancellable, &localError);
                if (!refs) {
                    qWarning() << "Failed to get list of installed refs for listing installed:" << localError->message;
                    continue;
                }

                resources.reserve(resources.size() + refs->len);
                for (uint i = 0; i < refs->len; i++) {
                    FlatpakInstalledRef *ref = FLATPAK_INSTALLED_REF(g_ptr_array_index(refs, i));
                    QString name = QString::fromUtf8(flatpak_installed_ref_get_appdata_name(ref));
                    if (name.endsWith(QLatin1String(".Debug")) || name.endsWith(QLatin1String(".Locale")) || name.endsWith(QLatin1String(".BaseApp"))
                        || name.endsWith(QLatin1String(".Docs")))
                        continue;

                    auto resource = getAppForInstalledRef(installation, ref);
                    if (resource->resourceType() == FlatpakResource::Runtime) {
                        resources.prepend(resource);
                    } else {
                        resources.append(resource);
                    }
                }
            }
            if (!resources.isEmpty())
                Q_EMIT stream->resourcesFound(resources);
            stream->finish();
        };

        if (isFetching()) {
            connect(this, &FlatpakBackend::initialized, stream, f);
        } else {
            QTimer::singleShot(0, this, f);
        }
        return stream;
    }

    auto stream = new ResultsStream(QStringLiteral("FlatpakStream"));
    auto f = [this, stream, filter]() {
        QVector<AbstractResource *> prioritary, rest;
        for (const auto &source : qAsConst(m_flatpakSources)) {
            QVector<FlatpakResource *> resources;
            if (source->m_pool) {
                resources = kTransform<QVector<FlatpakResource *>>(source->m_pool->search(filter.search), [this, &source](const auto &comp) {
                    return resourceForComponent(comp, source);
                });
            } else {
                resources = source->m_resources.values().toVector();
            }

            for (auto r : resources) {
                const bool matchById = r->appstreamId().compare(filter.search, Qt::CaseInsensitive) == 0;
                if (r->type() == AbstractResource::Technical && filter.state != AbstractResource::Upgradeable && !matchById) {
                    continue;
                }
                if (r->state() < filter.state)
                    continue;

                if (!filter.extends.isEmpty() && !r->extends().contains(filter.extends))
                    continue;

                if (!filter.mimetype.isEmpty() && !r->mimetypes().contains(filter.mimetype))
                    continue;

                if (filter.search.isEmpty() || matchById) {
                    rest += r;
                } else if (r->name().contains(filter.search, Qt::CaseInsensitive)) {
                    prioritary += r;
                } else if (r->comment().contains(filter.search, Qt::CaseInsensitive)) {
                    rest += r;
                }
            }
        }
        auto f = [this](AbstractResource *l, AbstractResource *r) {
            return flatpakResourceLessThan(l, r);
        };
        std::sort(rest.begin(), rest.end(), f);
        std::sort(prioritary.begin(), prioritary.end(), f);
        rest = prioritary + rest;
        if (!rest.isEmpty())
            Q_EMIT stream->resourcesFound(rest);
        stream->finish();
    };
    if (isFetching()) {
        connect(this, &FlatpakBackend::initialized, stream, f);
    } else {
        QTimer::singleShot(0, this, f);
    }
    return stream;
}

QVector<AbstractResource *> FlatpakBackend::resourcesByAppstreamName(const QString &name) const
{
    QVector<AbstractResource *> resources;
    const QString nameWithDesktop = name + QLatin1String(".desktop");
    for (const auto &source : m_flatpakSources) {
        if (source->m_pool) {
            const auto comps = source->m_pool->componentsById(name) + source->m_pool->componentsById(nameWithDesktop);
            resources << kTransform<QVector<AbstractResource *>>(comps, [this, source](const auto &comp) {
                return resourceForComponent(comp, source);
            });
        }
    }
    auto f = [this](AbstractResource *l, AbstractResource *r) {
        return flatpakResourceLessThan(l, r);
    };
    std::sort(resources.begin(), resources.end(), f);
    return resources;
}

ResultsStream *FlatpakBackend::findResourceByPackageName(const QUrl &url)
{
    if (url.scheme() == QLatin1String("appstream")) {
        const auto appstreamIds = AppStreamUtils::appstreamIds(url);
        if (appstreamIds.isEmpty())
            Q_EMIT passiveMessage(i18n("Malformed appstream url '%1'", url.toDisplayString()));
        else {
            auto stream = new ResultsStream(QStringLiteral("FlatpakStream"));
            auto f = [this, stream, appstreamIds]() {
                const auto resources = kAppend<QVector<AbstractResource *>>(appstreamIds, [this](const QString &appstreamId) {
                    return resourcesByAppstreamName(appstreamId);
                });
                if (!resources.isEmpty())
                    Q_EMIT stream->resourcesFound(resources);
                stream->finish();
            };

            if (isFetching()) {
                connect(this, &FlatpakBackend::initialized, stream, f);
            } else {
                QTimer::singleShot(0, this, f);
            }
            return stream;
        }
    }
    return new ResultsStream(QStringLiteral("FlatpakStream-packageName-void"), {});
}

FlatpakResource *FlatpakBackend::resourceForComponent(const AppStream::Component &component, const QSharedPointer<FlatpakSource> &source) const
{
    auto resource = source->m_resources.value(idForRefString(component.bundle(AppStream::Bundle::KindFlatpak).id()));
    if (resource) {
        return resource;
    }

    FlatpakResource *res = new FlatpakResource(component, source->installation(), const_cast<FlatpakBackend *>(this));
    res->setOrigin(source->name());
    res->setIconPath(source->appstreamIconsDir());
    res->updateFromAppStream();
    source->addResource(res);
    return res;
}

AbstractBackendUpdater *FlatpakBackend::backendUpdater() const
{
    return m_updater;
}

AbstractReviewsBackend *FlatpakBackend::reviewsBackend() const
{
    return m_reviews.data();
}

Transaction *FlatpakBackend::installApplication(AbstractResource *app, const AddonList &addons)
{
    Q_UNUSED(addons);

    FlatpakResource *resource = qobject_cast<FlatpakResource *>(app);

    if (resource->resourceType() == FlatpakResource::Source) {
        // Let source backend handle this
        FlatpakRemote *remote = m_sources->installSource(resource);
        if (remote) {
            resource->setState(AbstractResource::Installed);
            m_refreshAppstreamMetadataJobs++;
            // Make sure we update appstream metadata first
            // FIXME we have to let flatpak to return the remote as the one created by FlatpakSourcesBackend will not have appstream directory
            g_autoptr(FlatpakRemote) repo =
                flatpak_installation_get_remote_by_name(resource->installation(), flatpak_remote_get_name(remote), m_cancellable, nullptr);
            refreshAppstreamMetadata(resource->installation(), repo);
        }
        return nullptr;
    }

    FlatpakJobTransaction *transaction = new FlatpakJobTransaction(resource, Transaction::InstallRole);
    connect(transaction, &FlatpakJobTransaction::statusChanged, this, [this, resource](Transaction::Status status) {
        if (status == Transaction::Status::DoneStatus) {
            updateAppState(resource);
        }
    });
    return transaction;
}

Transaction *FlatpakBackend::installApplication(AbstractResource *app)
{
    return installApplication(app, {});
}

Transaction *FlatpakBackend::removeApplication(AbstractResource *app)
{
    FlatpakResource *resource = qobject_cast<FlatpakResource *>(app);

    if (resource->resourceType() == FlatpakResource::Source) {
        // Let source backend handle this
        if (m_sources->removeSource(resource->flatpakName())) {
            resource->setState(AbstractResource::None);
        }
        return nullptr;
    }

    FlatpakJobTransaction *transaction = new FlatpakJobTransaction(resource, Transaction::RemoveRole);

    connect(transaction, &FlatpakJobTransaction::statusChanged, this, [this, resource](Transaction::Status status) {
        if (status == Transaction::Status::DoneStatus) {
            updateAppSize(resource);
        }
    });
    return transaction;
}

void FlatpakBackend::checkForUpdates()
{
    for (auto installation : qAsConst(m_installations)) {
        // Load local updates, comparing current and latest commit
        loadLocalUpdates(installation);

        if (g_cancellable_is_cancelled(m_cancellable))
            break;
    }
}

QString FlatpakBackend::displayName() const
{
    return QStringLiteral("Flatpak");
}

#include "FlatpakBackend.moc"
