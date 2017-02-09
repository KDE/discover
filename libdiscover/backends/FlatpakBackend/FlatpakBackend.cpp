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
#include "FlatpakResource.h"
#include "FlatpakReviewsBackend.h"
#include "FlatpakTransaction.h"
#include "FlatpakSourcesBackend.h"

#include <resources/StandardBackendUpdater.h>
#include <resources/SourcesModel.h>
#include <Transaction/Transaction.h>
#include <Transaction/TransactionModel.h>

#include <AppStreamQt/bundle.h>
#include <AppStreamQt/component.h>
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
    , m_reviews(new FlatpakReviewsBackend(this))
    , m_fetching(false)
{
    g_autoptr(GError) error = nullptr;
    m_cancellable = g_cancellable_new();

    connect(m_updater, &StandardBackendUpdater::updatesCountChanged, this, &FlatpakBackend::updatesCountChanged);

    // Load flatpak installation
    if (!setupFlatpakInstallations(&error)) {
        qWarning() << "Failed to setup flatpak installations: " << error->message;
    } else {
        reloadPackageList();

        checkForUpdates();
    }

    QAction* updateAction = new QAction(this);
    updateAction->setIcon(QIcon::fromTheme(QStringLiteral("system-software-update")));
    updateAction->setText(i18nc("@action Checks the Internet for updates", "Check for Updates"));
    updateAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_R));
    connect(updateAction, &QAction::triggered, this, &FlatpakBackend::checkForUpdates);

    m_messageActions = QList<QAction*>() << updateAction;

    SourcesModel::global()->addSourcesBackend(new FlatpakSourcesBackend(this));
}

FlatpakBackend::~FlatpakBackend()
{
    g_object_unref(m_flatpakInstallationSystem);
    g_object_unref(m_flatpakInstallationUser);
    g_object_unref(m_cancellable);
}

FlatpakRef * FlatpakBackend::createFakeRef(FlatpakResource *resource)
{
    FlatpakRef *ref = nullptr;
    g_autoptr(GError) localError = nullptr;

    const QString id = QString::fromUtf8("%1/%2/%3/%4").arg(resource->typeAsString()).arg(resource->flatpakName()).arg(resource->arch()).arg(resource->branch());
    ref = flatpak_ref_parse(id.toStdString().c_str(), &localError);

    if (!ref) {
        qWarning() << "Failed to create fake ref: " << localError->message;
    }

    return ref;
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

    const QString runtimeId = QString::fromUtf8("runtime/%1/%2").arg(runtimeInfo.at(0)).arg(runtimeInfo.at(2));
    foreach (const QString &id, m_resources.keys()) {
        if (id.endsWith(runtimeId)) {
            runtime = m_resources.value(id);
            break;
        }
    }

    // TODO if runtime wasn't found, create a new one from available info

    return runtime;
}

void FlatpakBackend::addResource(FlatpakResource *resource)
{
    // Update app with all possible information we have
    if (!parseMetadataFromAppBundle(resource)) {
        qWarning() << "Failed to parse metadata from app bundle for " << resource->name();
    }

    auto installation = resource->scope() == FlatpakResource::System ? m_flatpakInstallationSystem : m_flatpakInstallationUser;
    updateAppState(installation, resource);

    if (resource->type() == FlatpakResource::DesktopApp) {
        if (!updateAppMetadata(installation, resource)) {
            qWarning() << "Failed to update" << resource->name() << "with installed metadata";
        }
    }

    updateAppSize(installation, resource);

    connect(resource, &FlatpakResource::stateChanged, this, &FlatpakBackend::updatesCountChanged);

    m_resources.insert(resource->uniqueId(), resource);
}

bool FlatpakBackend::compareAppFlatpakRef(FlatpakInstallation *flatpakInstallation, FlatpakResource *resource, FlatpakInstalledRef *ref)
{
    const QString arch = QString::fromUtf8(flatpak_ref_get_arch(FLATPAK_REF(ref)));
    const QString branch = QString::fromUtf8(flatpak_ref_get_branch(FLATPAK_REF(ref)));
    FlatpakResource::ResourceType appType = flatpak_ref_get_kind(FLATPAK_REF(ref)) == FLATPAK_REF_KIND_APP ? FlatpakResource::DesktopApp : FlatpakResource::Runtime;
    FlatpakResource::Scope appScope = flatpak_installation_get_is_user(flatpakInstallation) ? FlatpakResource::User : FlatpakResource::System;

    g_autofree gchar *appId = nullptr;

    if (appType == FlatpakResource::DesktopApp) {
        appId = g_strdup_printf("%s.desktop", flatpak_ref_get_name(FLATPAK_REF(ref)));
    } else {
        appId = g_strdup(flatpak_ref_get_name(FLATPAK_REF(ref)));
    }

    const QString uniqueId = QString::fromUtf8("%1/%2/%3/%4/%5/%6").arg(FlatpakResource::scopeAsString(appScope))
                                                                   .arg(QLatin1String("flatpak"))
                                                                   .arg(QString::fromUtf8(flatpak_installed_ref_get_origin(ref)))
                                                                   .arg(FlatpakResource::typeAsString(appType))
                                                                   .arg(QString::fromUtf8(appId))
                                                                   .arg(QString::fromUtf8(flatpak_ref_get_branch(FLATPAK_REF(ref))));

    // Compare uniqueId first then attempt to compare what we have
    if (resource->uniqueId() == uniqueId) {
        return true;
    }

    // Check if we have information about architecture and branch, otherwise compare names only
    // Happens with apps which don't have appstream metadata bug got here thanks to installed desktop file
    if (!resource->arch().isEmpty() && !resource->branch().isEmpty()) {
        return resource->arch() == arch && resource->branch() == branch && resource->flatpakName() == QString::fromUtf8(appId);
    }

    return (resource->flatpakName() == QString::fromUtf8(appId) || resource->flatpakName() == QString::fromUtf8(flatpak_ref_get_name(FLATPAK_REF(ref))));
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
            qWarning() << "No appstream dir for " << flatpak_remote_get_name(m_remote);
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
        qWarning() << "No " << appDirFileName << " appstream metadata found for " << source.name();
        return;
    }

    g_autoptr(AsMetadata) metadata = as_metadata_new();
    g_autoptr(GFile) file = g_file_new_for_path(appDirFileName.toStdString().c_str());
    as_metadata_set_format_style (metadata, AS_FORMAT_STYLE_COLLECTION);
    as_metadata_parse_file(metadata, file, AS_FORMAT_KIND_XML, &localError);
    if (localError) {
        qWarning() << "Failed to parse appstream metadata " << localError->message;
        return;
    }

    g_autoptr(GPtrArray) components = as_metadata_get_components(metadata);
    const FlatpakResource::Scope scope = flatpak_installation_get_is_user(flatpakInstallation) ? FlatpakResource::User : FlatpakResource::System;
    for (uint i = 0; i < components->len; i++) {
        AsComponent *component = AS_COMPONENT(g_ptr_array_index(components, i));
        AppStream::Component *appstreamComponent = new AppStream::Component(component);
        FlatpakResource *resource = new FlatpakResource(appstreamComponent, this);
        resource->setScope(scope);
        resource->setIconPath(appstreamDirPath);
        resource->setOrigin(source.name());
        addResource(resource);
    }
}

bool FlatpakBackend::loadInstalledApps(FlatpakInstallation *flatpakInstallation)
{
    QDir dir;
    QString pathExports;
    QString pathApps;
    g_autoptr(GFile) path = nullptr;

    if (!flatpakInstallation) {
        return false;
    }

    // List installed applications from installed desktop files
    path = flatpak_installation_get_path(flatpakInstallation);
    pathExports = QString::fromUtf8(g_file_get_path(path)) + QLatin1String("/exports/");
    pathApps = pathExports + QLatin1String("share/applications/");

    dir = QDir(pathApps);
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
                qWarning() << "Couldn't open " << fnDesktop << " :" << localError->message;
                continue;
            }

            as_metadata_parse_file(metadata, desktopFile, AS_FORMAT_KIND_DESKTOP_ENTRY, &localError);
            if (localError) {
                qWarning() << "Failed to parse appstream metadata " << localError->message;
                continue;
            }

            component = as_metadata_get_component(metadata);
            AppStream::Component *appstreamComponent = new AppStream::Component(component);
            FlatpakResource *resource = new FlatpakResource(appstreamComponent, this);

            resource->setScope(flatpak_installation_get_is_user(flatpakInstallation) ? FlatpakResource::User : FlatpakResource::System);
            resource->setIconPath(pathExports);
            resource->setType(FlatpakResource::DesktopApp);
            resource->setState(AbstractResource::Installed);

            // Go through apps we already know about from appstream metadata
            bool resourceExists = false;
            foreach (FlatpakResource *res, m_resources) {
                // Compare the only information we have
                if (res->appstreamId() == QString::fromUtf8("%1.desktop").arg(resource->appstreamId()) && res->name() == resource->name()) {
                    resourceExists = true;
                    res->setScope(resource->scope());
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
        qWarning() << "Failed to get list of installed refs for listing updates: " << localError->message;
        return;
    }

    for (uint i = 0; i < refs->len; i++) {
        FlatpakInstalledRef *ref = FLATPAK_INSTALLED_REF(g_ptr_array_index(refs, i));
        const gchar *latestCommit = flatpak_installed_ref_get_latest_commit(ref);

        if (!latestCommit) {
            qWarning() << "Couldn'g get latest commit for " << flatpak_ref_get_name(FLATPAK_REF(ref));
        }

        const gchar *commit = flatpak_ref_get_commit(FLATPAK_REF(ref));
        if (g_strcmp0(commit, latestCommit) == 0) {
            continue;
        }

        FlatpakResource *resource = getAppForInstalledRef(flatpakInstallation, ref);
        if (resource) {
            resource->setState(AbstractResource::Upgradeable);
        }
    }
}

void FlatpakBackend::loadRemoteUpdates(FlatpakInstallation *flatpakInstallation)
{
    g_autoptr(GError) localError = nullptr;
    g_autoptr(GPtrArray) refs = nullptr;

    refs = flatpak_installation_list_installed_refs_for_update(flatpakInstallation, m_cancellable, &localError);
    if (!refs) {
        qWarning() << "Failed to get list of installed refs for listing updates: " << localError->message;
        return;
    }

    for (uint i = 0; i < refs->len; i++) {
        FlatpakInstalledRef *ref = FLATPAK_INSTALLED_REF(g_ptr_array_index(refs, i));
        FlatpakResource *resource = getAppForInstalledRef(flatpakInstallation, ref);
        if (resource) {
            resource->setState(AbstractResource::Upgradeable);
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
            qWarning() << "Failed to parse " << bundle.id() << localError->message;
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

    // Load applications from appstream metadata
    if (!loadAppsFromAppstreamData(m_flatpakInstallationSystem)) {
        qWarning() << "Failed to load packages from appstream data from system installation";
    }

    if (!loadAppsFromAppstreamData(m_flatpakInstallationUser)) {
        qWarning() << "Failed to load packages from appstream data from user installation";
    }

    // Load installed applications and update existing resources with info from installed application
    if (!loadInstalledApps(m_flatpakInstallationSystem)) {
        qWarning() << "Failed to load installed packages from system installation";
    }

    if (!loadInstalledApps(m_flatpakInstallationUser)) {
        qWarning() << "Failed to load installed packages from user installation";
    }

    setFetching(false);
}

bool FlatpakBackend::setupFlatpakInstallations(GError **error)
{
    m_flatpakInstallationSystem = flatpak_installation_new_system(m_cancellable, error);
    if (!m_flatpakInstallationSystem) {
        return false;
    }

    m_flatpakInstallationUser = flatpak_installation_new_user(m_cancellable, error);
    if (!m_flatpakInstallationUser) {
        return false;
    }

    return true;
}

void FlatpakBackend::updateAppInstalledMetadata(FlatpakInstalledRef *installedRef, FlatpakResource *resource)
{
    // Update the rest
    resource->updateFromRef(FLATPAK_REF(installedRef));
    resource->setOrigin(QString::fromUtf8(flatpak_installed_ref_get_origin(installedRef)));
    resource->setSize(flatpak_installed_ref_get_installed_size(installedRef));
    resource->setState(AbstractResource::Installed);
}

bool FlatpakBackend::updateAppMetadata(FlatpakInstallation* flatpakInstallation, FlatpakResource *resource)
{
    QByteArray metadataContent;
    g_autoptr(GBytes) data = nullptr;
    g_autoptr(GFile) installationPath = nullptr;
    g_autoptr(GError) localError = nullptr;

    if (resource->type() != FlatpakResource::DesktopApp) {
        return true;
    }

    installationPath = flatpak_installation_get_path(flatpakInstallation);
    const QString path = QString::fromUtf8(g_file_get_path(installationPath)) + QString::fromUtf8("/app/%1/%2/%3/active/metadata").arg(resource->flatpakName()).arg(resource->arch()).arg(resource->branch());

    if (QFile::exists(path)) {
        QFile file(path);
        if (file.open(QFile::ReadOnly | QFile::Text)) {
            metadataContent = file.readAll();
        }
    } else {
        g_autoptr(FlatpakRef) fakeRef = nullptr;

        if (resource->origin().isEmpty()) {
            qWarning() << "Failed to get metadata file because of missing origin";
            return false;
        }

        fakeRef = createFakeRef(resource);
        if (!fakeRef) {
            return false;
        }

        data = flatpak_installation_fetch_remote_metadata_sync(flatpakInstallation, resource->origin().toStdString().c_str(), fakeRef, m_cancellable, &localError);
        if (data) {
            gsize len = 0;
            metadataContent = QByteArray((char *)g_bytes_get_data(data, &len));
        } else {
            qWarning() << "Failed to get metadata file: " << localError->message;
            return false;
        }
    }

    if (metadataContent.isEmpty()) {
        qWarning() << "Failed to get metadata file";
        return false;
    }

    // Save the content to temporary file
    QTemporaryFile tempFile;
    tempFile.setAutoRemove(false);
    if (!tempFile.open()) {
        qWarning() << "Failed to get metadata file";
        return false;
    }

    tempFile.write(metadataContent);
    tempFile.close();

    // Parse the temporary file
    QSettings setting(tempFile.fileName(), QSettings::NativeFormat);
    setting.beginGroup(QLatin1String("Application"));
    // Set the runtime in form of name/arch/version which can be later easily parsed
    resource->setRuntime(setting.value(QLatin1String("runtime")).toString());
    // TODO get more information?

    tempFile.remove();

    return true;
}

bool FlatpakBackend::updateAppSize(FlatpakInstallation *flatpakInstallation, FlatpakResource *resource)
{
    guint64 downloadSize = 0;
    guint64 installedSize = 0;

    // Check if the size is already set, we should also distiguish between download and installed size,
    // right now it doesn't matter whether we get size for installed or not installed app, but if we
    // start making difference then for not installed app check download and install size separately

    // if (resource->isInstalled()) {
        // The size appears to be already set (from updateAppInstalledMetadata() apparently)
        if (resource->size() > 0) {
            return true;
        }
    // } else {
    //  TODO check download and installed size separately
    // }

    // Check if we know the needed runtime which is needed for calculating the size
    if (resource->runtime().isEmpty()) {
        if (!updateAppMetadata(flatpakInstallation, resource)) {
            qWarning() << "Failed to get runtime for " << resource->name() << " needed for calculating of size";
            return false;
        }
    }

    // Calculate the runtime size
    FlatpakResource *runtime = nullptr;
    if (resource->state() == AbstractResource::None && resource->type() == FlatpakResource::DesktopApp) {
        runtime = getRuntimeForApp(resource);
        if (runtime) {
            // Re-check runtime state if case a new one was created
            updateAppState(flatpakInstallation, runtime);

            if (!runtime->isInstalled()) {
                if (!updateAppSize(flatpakInstallation, runtime)) {
                    qWarning() << "Failed to get runtime size needed for total size of " << resource->name();
                    return false;
                }
            }
        }
    }

    if (resource->isInstalled()) {
        g_autoptr(FlatpakInstalledRef) ref = nullptr;
        ref = getInstalledRefForApp(flatpakInstallation, resource);
        if (!ref) {
            qWarning() << "Failed to get installed size of " << resource->name();
            return false;
        }
        resource->setSize(flatpak_installed_ref_get_installed_size(ref));
    } else {
        g_autoptr(FlatpakRef) ref = nullptr;
        g_autoptr(GError) localError = nullptr;

        if (resource->origin().isEmpty()) {
            qWarning() << "Failed to get size of " << resource->name() << " because of missing origin";
        }

        ref = createFakeRef(resource);
        if (!ref) {
            return false;
        }

        if (!flatpak_installation_fetch_remote_size_sync(flatpakInstallation, resource->origin().toStdString().c_str(),
                                                         ref, &downloadSize, &installedSize, m_cancellable, &localError)) {
            qWarning() << "Failed to get remote size of " << resource->name() << ": " << localError->message;
            return false;
        }

        // TODO: What size do we want to show (installed vs download)? Do we want to show app size + runtime size if runtime is not installed?
        if (runtime && !runtime->isInstalled()) {
            resource->setSize(runtime->size() + installedSize);
        } else {
            resource->setSize(installedSize);
        }
    }

    return true;
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
        if (qobject_cast<FlatpakResource*>(r)->type() == FlatpakResource::Runtime && filter.state != AbstractResource::Upgradeable) {
            continue;
        }

        if (r->name().contains(filter.search, Qt::CaseInsensitive) || r->comment().contains(filter.search, Qt::CaseInsensitive)) {
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
                if (res->appstreamId() == url.host())
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
    return m_reviews;
}

FlatpakInstallation * FlatpakBackend::flatpakInstallationForAppScope(FlatpakResource::Scope appScope) const
{
    if (appScope == FlatpakResource::Scope::System) {
        return m_flatpakInstallationSystem;
    } else {
        return m_flatpakInstallationUser;
    }
}

void FlatpakBackend::installApplication(AbstractResource *app, const AddonList &addons)
{
    FlatpakTransaction *transaction = nullptr;
    FlatpakResource *resource = qobject_cast<FlatpakResource*>(app);
    FlatpakInstallation *installation = resource->scope() == FlatpakResource::System ? m_flatpakInstallationSystem : m_flatpakInstallationUser;

    FlatpakResource *runtime = getRuntimeForApp(resource);
    if (runtime && !runtime->isInstalled()) {
        transaction = new FlatpakTransaction(installation, resource, runtime, addons, Transaction::InstallRole);
    } else {
        transaction = new FlatpakTransaction(installation, resource, addons, Transaction::InstallRole);
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
    FlatpakInstallation *installation = resource->scope() == FlatpakResource::System ? m_flatpakInstallationSystem : m_flatpakInstallationUser;
    new FlatpakTransaction(installation, resource, Transaction::RemoveRole);
}

void FlatpakBackend::checkForUpdates()
{
    // TODO update size

    // Load local updates, comparing current and latest commit
    loadLocalUpdates(m_flatpakInstallationSystem);
    loadLocalUpdates(m_flatpakInstallationUser);

    // Load updates from remote repositories
    loadRemoteUpdates(m_flatpakInstallationSystem);
    loadRemoteUpdates(m_flatpakInstallationUser);
}

FlatpakRemote * FlatpakBackend::flatpakRemoteByUrl(const QString& url, FlatpakInstallation* installation) const
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

AbstractResource * FlatpakBackend::resourceForFile(const QUrl &url)
{
    if (!url.path().endsWith(QLatin1String(".flatpakref")) || !url.isLocalFile())
        return nullptr;

    auto installation = m_flatpakInstallationSystem;
    QSettings settings(url.toLocalFile(), QSettings::NativeFormat);
    const QString refurl = settings.value(QStringLiteral("Flatpak Ref/Url")).toString();
    FlatpakRemote* remote = flatpakRemoteByUrl(refurl, installation);

    if (!remote) {
        const auto res = QMessageBox::question(nullptr, i18n("Add Remote"),
                                            i18n("Would you like to add remote '%1'?\n\nFrom: %2\nWith GPG key=%3...",
                                                    settings.value(QStringLiteral("Flatpak Ref/Title")).toString(),
                                                    refurl,
                                                    settings.value(QStringLiteral("Flatpak Ref/GPGKey")).toString().left(23)),
                                            QMessageBox::StandardButtons(QMessageBox::Yes | QMessageBox::No));
        if (res != QMessageBox::Yes) {
            return nullptr;
        }
    }

    g_autoptr(GError) error = NULL;
    g_autoptr(FlatpakRemoteRef) remoteRef = nullptr;
    {
        QFile f(url.toLocalFile());
        if (!f.open(QFile::ReadOnly | QFile::Text))
            return nullptr;

        QByteArray contents = f.readAll();

        g_autoptr(GBytes) bytes = g_bytes_new (contents.data(), contents.size());

        remoteRef = flatpak_installation_install_ref_file (installation, bytes, m_cancellable, &error);
        if (!remoteRef)
            return nullptr;
    }

    const auto remoteName = flatpak_remote_ref_get_remote_name(remoteRef);
    remote = flatpak_installation_get_remote_by_name(installation, remoteName, m_cancellable, &error);

    auto ref = FLATPAK_REF(remoteRef);

    auto c = new AppStream::Component();
    c->setName(QString::fromUtf8(flatpak_ref_get_name(ref)));
    auto resource = new FlatpakResource(c, this);
    resource->updateFromRef(ref);
    resource->setOrigin(QString::fromUtf8(remoteName));
    addResource(resource);
    return resource;
}

#include "FlatpakBackend.moc"
