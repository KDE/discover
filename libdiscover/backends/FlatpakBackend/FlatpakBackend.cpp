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
#include "FlatpakFetchUpdatesJob.h"
#include "FlatpakResource.h"
#include "FlatpakSourcesBackend.h"
#include "FlatpakTransaction.h"

#include <resources/StandardBackendUpdater.h>
#include <resources/SourcesModel.h>
#include <Transaction/Transaction.h>
#include <Transaction/TransactionModel.h>
#include <appstream/OdrsReviewsBackend.h>
#include <appstream/AppStreamIntegration.h>

#include <AppStreamQt/bundle.h>
#include <AppStreamQt/component.h>
#include <AppStreamQt/icon.h>
#include <AppStream/appstream.h>

#include <KAboutData>
#include <KLocalizedString>
#include <KPluginFactory>
#include <KConfigGroup>
#include <KSharedConfig>

#include <QAction>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QMessageBox>
#include <QSettings>
#include <QThread>
#include <QTimer>
#include <QTextStream>
#include <QTemporaryFile>

#include <glib.h>

MUON_BACKEND_PLUGIN(FlatpakBackend)

FlatpakBackend::FlatpakBackend(QObject* parent)
    : AbstractResourcesBackend(parent)
    , m_updater(new StandardBackendUpdater(this))
    , m_reviews(AppStreamIntegration::global()->reviews())
    , m_fetching(false)
{
    g_autoptr(GError) error = nullptr;
    m_cancellable = g_cancellable_new();

    connect(m_updater, &StandardBackendUpdater::updatesCountChanged, this, &FlatpakBackend::updatesCountChanged);

    // Load flatpak installation
    if (!setupFlatpakInstallations(&error)) {
        qWarning() << "Failed to setup flatpak installations:" << error->message;
    } else {
        reloadPackageList();

        checkForUpdates();

        m_sources = new FlatpakSourcesBackend(m_installations, this);
        SourcesModel::global()->addSourcesBackend(m_sources);
    }

    connect(m_reviews.data(), &OdrsReviewsBackend::ratingsReady, this, &FlatpakBackend::announceRatingsReady);
}

FlatpakBackend::~FlatpakBackend()
{
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

FlatpakInstalledRef * FlatpakBackend::getInstalledRefForApp(FlatpakInstallation *flatpakInstallation, FlatpakResource *resource)
{
    AppStream::Component *component = resource->appstreamComponent();
    AppStream::Component::Kind appKind = component->kind();
    FlatpakInstalledRef *ref = nullptr;
    GPtrArray *installedApps = nullptr;
    g_autoptr(GError) localError = nullptr;

    if (!flatpakInstallation) {
        return ref;
    }

    ref = flatpak_installation_get_installed_ref(flatpakInstallation,
                                                 resource->type() == FlatpakResource::DesktopApp ? FLATPAK_REF_KIND_APP : FLATPAK_REF_KIND_RUNTIME,
                                                 resource->flatpakName().toStdString().c_str(),
                                                 resource->arch().toStdString().c_str(),
                                                 resource->branch().toStdString().c_str(),
                                                 m_cancellable, &localError);

    // If we found installed ref this way, we can return it
    if (ref) {
        return ref;
    }

    // Otherwise go through all installed apps and try to match info we have
    installedApps = flatpak_installation_list_installed_refs_by_kind(flatpakInstallation,
                                                                     appKind == AppStream::Component::KindDesktopApp ? FLATPAK_REF_KIND_APP : FLATPAK_REF_KIND_RUNTIME,
                                                                     m_cancellable, &localError);
    if (!installedApps) {
        return ref;
    }

    for (uint i = 0; i < installedApps->len; i++) {
        FlatpakInstalledRef *installedRef = FLATPAK_INSTALLED_REF(g_ptr_array_index(installedApps, i));

        // Check if the installed_reference and app_id are the same and update the app with installed metadata
        if (compareAppFlatpakRef(flatpakInstallation, resource, installedRef)) {
            return installedRef;
        }
    }

    // We found nothing, return nullptr
    return ref;
}

FlatpakResource * FlatpakBackend::getAppForInstalledRef(FlatpakInstallation *flatpakInstallation, FlatpakInstalledRef *ref)
{
    foreach (FlatpakResource *resource, m_resources) {
        if (compareAppFlatpakRef(flatpakInstallation, resource, ref)) {
            return resource;
        }
    }

    return nullptr;
}

FlatpakResource * FlatpakBackend::getRuntimeForApp(FlatpakResource *resource)
{
    FlatpakResource *runtime = nullptr;
    const auto runtimeInfo = resource->runtime().split(QLatin1Char('/'));

    if (runtimeInfo.count() != 3) {
        return runtime;
    }

    const QString runtimeId = QStringLiteral("runtime/") + runtimeInfo.at(0) + QLatin1Char('/') + runtimeInfo.at(2);

    foreach (const QString &id, m_resources.keys()) {
        if (id.endsWith(runtimeId)) {
            runtime = m_resources.value(id);
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
    AppStream::Component *asComponent = nullptr;

    file = g_file_new_for_path(url.toLocalFile().toStdString().c_str());
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
        g_autofree gchar *appstreamContent = g_strndup((char*)data, len);

        g_autoptr(AsMetadata) metadata = as_metadata_new();
        as_metadata_set_format_style(metadata, AS_FORMAT_STYLE_COLLECTION);
        as_metadata_parse(metadata, appstreamContent, AS_FORMAT_KIND_XML, &localError);
        if (localError) {
            qWarning() << "Failed to parse appstream metadata:" << localError->message;
            return nullptr;
        }

        g_autoptr(GPtrArray) components = as_metadata_get_components(metadata);
        if (g_ptr_array_index(components, 0)) {
            asComponent = new AppStream::Component(AS_COMPONENT(g_ptr_array_index(components, 0)));
        } else {
            qWarning() << "Failed to parse appstream metadata";
            return nullptr;
        }
    } else {
        AsComponent *component = as_component_new();
        asComponent = new AppStream::Component(component);
        qWarning() << "No appstream metadata in bundle";
    }

    gsize len = 0;
    g_autoptr(GBytes) iconData = nullptr;
    g_autoptr(GBytes) metadata = nullptr;
    FlatpakResource *resource = new FlatpakResource(asComponent, preferredInstallation(), this);

    metadata = flatpak_bundle_ref_get_metadata(bundleRef);
    QByteArray metadataContent = QByteArray((char *)g_bytes_get_data(metadata, &len));
    if (!updateAppMetadata(resource, metadataContent)) {
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

    resource->setDownloadSize(0);
    resource->setInstalledSize(flatpak_bundle_ref_get_installed_size(bundleRef));
    resource->setPropertyState(FlatpakResource::DownloadSize, FlatpakResource::AlreadyKnown);
    resource->setPropertyState(FlatpakResource::InstalledSize, FlatpakResource::AlreadyKnown);
    resource->setFlatpakFileType(QStringLiteral("flatpak"));
    resource->setOrigin(QString::fromUtf8(flatpak_bundle_ref_get_origin(bundleRef)));
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

    AsComponent *component = as_component_new();
    as_component_add_url(component, AS_URL_KIND_HOMEPAGE, settings.value(QStringLiteral("Flatpak Ref/Homepage")).toString().toStdString().c_str());
    as_component_set_description(component, settings.value(QStringLiteral("Flatpak Ref/Description")).toString().toStdString().c_str(), nullptr);
    as_component_set_name(component, settings.value(QStringLiteral("Flatpak Ref/Title")).toString().toStdString().c_str(), nullptr);
    as_component_set_summary(component, settings.value(QStringLiteral("Flatpak Ref/Comment")).toString().toStdString().c_str(), nullptr);
    const QString iconUrl = settings.value(QStringLiteral("Flatpak Ref/Icon")).toString();
    if (!iconUrl.isEmpty()) {
        AsIcon *icon = as_icon_new();
        as_icon_set_kind(icon, AS_ICON_KIND_REMOTE);
        as_icon_set_url(icon, iconUrl.toStdString().c_str());
        as_component_add_icon(component, icon);
    }

    AppStream::Component *asComponent = new AppStream::Component(component);
    auto resource = new FlatpakResource(asComponent, preferredInstallation(), this);
    resource->setFlatpakFileType(QStringLiteral("flatpakref"));
    resource->setOrigin(QString::fromUtf8(remoteName));
    resource->updateFromRef(ref);
    addResource(resource);
    return resource;
}

FlatpakResource * FlatpakBackend::addSourceFromFlatpakRepo(const QUrl &url)
{
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

    AsComponent *component = as_component_new();
    as_component_add_url(component, AS_URL_KIND_HOMEPAGE, settings.value(QStringLiteral("Flatpak Repo/Homepage")).toString().toStdString().c_str());
    as_component_set_summary(component, settings.value(QStringLiteral("Flatpak Repo/Comment")).toString().toStdString().c_str(), nullptr);
    as_component_set_description(component, settings.value(QStringLiteral("Flatpak Repo/Description")).toString().toStdString().c_str(), nullptr);
    as_component_set_name(component, title.toStdString().c_str(), nullptr);
    const QString iconUrl = settings.value(QStringLiteral("Flatpak Repo/Icon")).toString();
    if (!iconUrl.isEmpty()) {
        AsIcon *icon = as_icon_new();
        as_icon_set_kind(icon, AS_ICON_KIND_REMOTE);
        as_icon_set_url(icon, iconUrl.toStdString().c_str());
        as_component_add_icon(component, icon);
    }

    AppStream::Component *asComponent = new AppStream::Component(component);
    auto resource = new FlatpakResource(asComponent, preferredInstallation(), this);
    // Use metadata only for stuff which are not common for all resources
    resource->addMetadata(QStringLiteral("gpg-key"), gpgKey);
    resource->addMetadata(QStringLiteral("repo-url"), repoUrl);
    resource->setBranch(settings.value(QStringLiteral("Flatpak Repo/DefaultBranch")).toString());
    resource->setFlatpakName(url.fileName().remove(QStringLiteral(".flatpakrepo")));
    resource->setType(FlatpakResource::Source);

    auto repo = flatpak_installation_get_remote_by_name(preferredInstallation(), resource->flatpakName().toStdString().c_str(), m_cancellable, nullptr);
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

bool FlatpakBackend::compareAppFlatpakRef(FlatpakInstallation *flatpakInstallation, FlatpakResource *resource, FlatpakInstalledRef *ref)
{
    const QString arch = QString::fromUtf8(flatpak_ref_get_arch(FLATPAK_REF(ref)));
    const QString branch = QString::fromUtf8(flatpak_ref_get_branch(FLATPAK_REF(ref)));
    const FlatpakResource::ResourceType appType = flatpak_ref_get_kind(FLATPAK_REF(ref)) == FLATPAK_REF_KIND_APP ? FlatpakResource::DesktopApp : FlatpakResource::Runtime;

    const QString name = QLatin1String(flatpak_ref_get_name(FLATPAK_REF(ref)));
    const QString appId = appType == FlatpakResource::DesktopApp ?
            QLatin1String(flatpak_ref_get_name(FLATPAK_REF(ref))) + QStringLiteral(".desktop") :
            name;

    const QString uniqueId = QStringLiteral("%1/%2/%3/%4/%5/%6").arg(FlatpakResource::installationPath(flatpakInstallation))
                                                                   .arg(QLatin1String("flatpak"))
                                                                   .arg(QString::fromUtf8(flatpak_installed_ref_get_origin(ref)))
                                                                   .arg(FlatpakResource::typeAsString(appType))
                                                                   .arg(appId)
                                                                   .arg(branch);

    // Compare uniqueId first then attempt to compare what we have
    if (resource->uniqueId() == uniqueId) {
        return true;
    }

    // Check if we have information about architecture and branch, otherwise compare names only
    // Happens with apps which don't have appstream metadata bug got here thanks to installed desktop file
    if (!resource->arch().isEmpty() && !resource->branch().isEmpty()) {
        return resource->arch() == arch && resource->branch() == branch && (resource->flatpakName() == appId ||
                                                                            resource->flatpakName() == name);
    }

    return resource->flatpakName() == appId || resource->flatpakName() == name;
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

bool FlatpakBackend::loadAppsFromAppstreamData(FlatpakInstallation *flatpakInstallation)
{
    Q_ASSERT(flatpakInstallation);

    g_autoptr(GPtrArray) remotes = flatpak_installation_list_remotes(flatpakInstallation, m_cancellable, nullptr);
    if (!remotes) {
        return false;
    }

    for (uint i = 0; i < remotes->len; i++) {
        FlatpakRemote *remote = FLATPAK_REMOTE(g_ptr_array_index(remotes, i));
        integrateRemote(flatpakInstallation, remote);
    }

    return true;
}

void FlatpakBackend::integrateRemote(FlatpakInstallation *flatpakInstallation, FlatpakRemote *remote)
{
    g_autoptr(GError) localError = nullptr;

    FlatpakSource source(remote);
    if (!source.isEnabled() || flatpak_remote_get_noenumerate(remote)) {
        return;
    }

    const QString appstreamDirPath = source.appstreamDir();
    const QString appDirFileName = appstreamDirPath + QLatin1String("/appstream.xml.gz");
    if (!QFile::exists(appDirFileName)) {
        qWarning() << "No" << appDirFileName << "appstream metadata found for" << source.name();
        return;
    }

    g_autoptr(AsMetadata) metadata = as_metadata_new();
    g_autoptr(GFile) file = g_file_new_for_path(appDirFileName.toStdString().c_str());
    as_metadata_set_format_style (metadata, AS_FORMAT_STYLE_COLLECTION);
    as_metadata_parse_file(metadata, file, AS_FORMAT_KIND_XML, &localError);
    if (localError) {
        qWarning() << "Failed to parse appstream metadata" << localError->message;
        return;
    }

    g_autoptr(GPtrArray) components = as_metadata_get_components(metadata);
    for (uint i = 0; i < components->len; i++) {
        AsComponent *component = AS_COMPONENT(g_ptr_array_index(components, i));
        AppStream::Component *appstreamComponent = new AppStream::Component(component);
        FlatpakResource *resource = new FlatpakResource(appstreamComponent, flatpakInstallation, this);
        resource->setIconPath(appstreamDirPath);
        resource->setOrigin(source.name());
        addResource(resource);
    }
}

bool FlatpakBackend::loadInstalledApps(FlatpakInstallation *flatpakInstallation)
{
    Q_ASSERT(flatpakInstallation);

    // List installed applications from installed desktop files
    const QString pathExports = FlatpakResource::installationPath(flatpakInstallation) + QLatin1String("/exports/");
    const QString pathApps = pathExports + QLatin1String("share/applications/");

    const QDir dir(pathApps);
    if (dir.exists()) {
        foreach (const QString &file, dir.entryList(QDir::NoDotAndDotDot | QDir::Files)) {
            QString fnDesktop;
            AsComponent *component;
            g_autoptr(GError) localError = nullptr;
            g_autoptr(GFile) desktopFile = nullptr;
            g_autoptr(AsMetadata) metadata = as_metadata_new();

            if (file == QLatin1String("mimeinfo.cache")) {
                continue;
            }

            fnDesktop = pathApps + file;
            desktopFile = g_file_new_for_path(fnDesktop.toStdString().c_str());
            if (!desktopFile) {
                qWarning() << "Couldn't open" << fnDesktop << " :" << localError->message;
                continue;
            }

            as_metadata_parse_file(metadata, desktopFile, AS_FORMAT_KIND_DESKTOP_ENTRY, &localError);
            if (localError) {
                qWarning() << "Failed to parse appstream metadata" << localError->message;
                continue;
            }

            component = as_metadata_get_component(metadata);
            AppStream::Component *appstreamComponent = new AppStream::Component(component);
            FlatpakResource *resource = new FlatpakResource(appstreamComponent, flatpakInstallation, this);

            resource->setIconPath(pathExports);
            resource->setType(FlatpakResource::DesktopApp);
            resource->setState(AbstractResource::Installed);

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

void FlatpakBackend::loadRemoteUpdates(FlatpakInstallation *flatpakInstallation)
{
    FlatpakFetchUpdatesJob *job = new FlatpakFetchUpdatesJob(flatpakInstallation);
    connect(job, &FlatpakFetchUpdatesJob::finished, job, &FlatpakFetchUpdatesJob::deleteLater);
    connect(job, &FlatpakFetchUpdatesJob::jobFetchUpdatesFinished, this, &FlatpakBackend::onFetchUpdatesFinished);
    job->start();
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
    AppStream::Bundle bundle = resource->appstreamComponent()->bundle(AppStream::Bundle::KindFlatpak);

    // Get arch/branch/commit/name from FlatpakRef
    if (!bundle.isEmpty()) {
        ref = flatpak_ref_parse(bundle.id().toStdString().c_str(), &localError);
        if (!ref) {
            qWarning() << "Failed to parse" << bundle.id() << localError->message;
            return false;
        } else {
            resource->updateFromRef(ref);
        }
    }

    return true;
}

void FlatpakBackend::reloadPackageList()
{
    setFetching(true);

    for (auto installation : qAsConst(m_installations)) {
        // Load applications from appstream metadata
        if (!loadAppsFromAppstreamData(installation)) {
            qWarning() << "Failed to load packages from appstream data from installation" << installation;
        }

        // Load installed applications and update existing resources with info from installed application
        if (!loadInstalledApps(installation)) {
            qWarning() << "Failed to load installed packages from installation" << installation;
        }
    }

    setFetching(false);
}

bool FlatpakBackend::setupFlatpakInstallations(GError **error)
{
    GPtrArray *installations = flatpak_get_system_installations(m_cancellable, error);
    for (uint i = 0; i < installations->len; i++) {
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
        FlatpakFetchDataJob *job = new FlatpakFetchDataJob(flatpakInstallation, resource, FlatpakFetchDataJob::FetchMetadata);
        connect(job, &FlatpakFetchDataJob::finished, job, &FlatpakFetchDataJob::deleteLater);
        connect(job, &FlatpakFetchDataJob::jobFetchMetadataFinished, this, &FlatpakBackend::onFetchMetadataFinished);
        job->start();
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

        FlatpakFetchDataJob *job = new FlatpakFetchDataJob(flatpakInstallation, resource, FlatpakFetchDataJob::FetchSize);
        connect(job, &FlatpakFetchDataJob::finished, job, &FlatpakFetchDataJob::deleteLater);
        connect(job, &FlatpakFetchDataJob::jobFetchSizeFinished, this, &FlatpakBackend::onFetchSizeFinished);
        connect(job, &FlatpakFetchDataJob::jobFetchSizeFailed, [resource] () {
            resource->setPropertyState(FlatpakResource::DownloadSize, FlatpakResource::UnknownOrFailed);
            resource->setPropertyState(FlatpakResource::InstalledSize, FlatpakResource::UnknownOrFailed);
        });
        job->start();
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
    QVector<AbstractResource*> ret;

    foreach(AbstractResource* r, m_resources) {
        if (r->isTechnical() && filter.state != AbstractResource::Upgradeable) {
            continue;
        }

        if (r->state()<filter.state)
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

void FlatpakBackend::installApplication(AbstractResource *app, const AddonList &addons)
{
    Q_UNUSED(addons);

    FlatpakResource *resource = qobject_cast<FlatpakResource*>(app);

    if (resource->type() == FlatpakResource::Source) {
        // Let source backend handle this
        FlatpakRemote *remote = m_sources->installSource(resource);
        if (remote) {
            resource->setState(AbstractResource::Installed);
            integrateRemote(preferredInstallation(), remote);
        }
        return;
    }

    FlatpakTransaction *transaction = nullptr;
    FlatpakInstallation *installation = resource->installation();

    if (resource->propertyState(FlatpakResource::RequiredRuntime) == FlatpakResource::NotKnownYet && resource->type() == FlatpakResource::DesktopApp) {
        transaction = new FlatpakTransaction(installation, resource, Transaction::InstallRole, true);
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
            transaction = new FlatpakTransaction(installation, resource, runtime, Transaction::InstallRole);
        } else {
            transaction = new FlatpakTransaction(installation, resource, Transaction::InstallRole);
        }
    }

    connect(transaction, &FlatpakTransaction::statusChanged, [this, installation, resource] (Transaction::Status status) {
        if (status == Transaction::Status::DoneStatus) {
            updateAppState(installation, resource);
        }
    });
}

void FlatpakBackend::installApplication(AbstractResource *app)
{
    installApplication(app, {});
}

void FlatpakBackend::removeApplication(AbstractResource *app)
{
    FlatpakResource *resource = qobject_cast<FlatpakResource*>(app);

    if (resource->type() == FlatpakResource::Source) {
        // Let source backend handle this
        if (m_sources->removeSource(resource->flatpakName())) {
            resource->setState(AbstractResource::None);
        }
        return;
    }

    FlatpakInstallation *installation = resource->installation();
    FlatpakTransaction *transaction = new FlatpakTransaction(installation, resource, Transaction::RemoveRole);

    connect(transaction, &FlatpakTransaction::statusChanged, [this, installation, resource] (Transaction::Status status) {
        if (status == Transaction::Status::DoneStatus) {
            updateAppSize(installation, resource);
        }
    });
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
    if ((!url.path().endsWith(QLatin1String(".flatpakref")) && !url.path().endsWith(QLatin1String(".flatpak")) && !url.path().endsWith(QLatin1String(".flatpakrepo"))) || !url.isLocalFile()) {
        return nullptr;
    }

    FlatpakResource *resource = nullptr;
    if (url.path().endsWith(QLatin1String(".flatpak"))) {
        resource = addAppFromFlatpakBundle(url);
    } else if (url.path().endsWith(QLatin1String(".flatpakref"))) {
        resource = addAppFromFlatpakRef(url);
    } else {
        resource = addSourceFromFlatpakRepo(url);
    }

    return resource;
}

#include "FlatpakBackend.moc"
