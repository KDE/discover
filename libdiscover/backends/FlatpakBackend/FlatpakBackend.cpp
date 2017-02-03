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

#include <KAboutData>
#include <KLocalizedString>
#include <KPluginFactory>
#include <KConfigGroup>
#include <KSharedConfig>

#include <QAction>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QSettings>
#include <QThread>
#include <QTimer>
#include <QTextStream>
#include <QTemporaryFile>

MUON_BACKEND_PLUGIN(FlatpakBackend)

FlatpakBackend::FlatpakBackend(QObject* parent)
    : AbstractResourcesBackend(parent)
    , m_updater(new StandardBackendUpdater(this))
    , m_reviews(new FlatpakReviewsBackend(this))
    , m_fetching(true)
{
    g_autoptr(GCancellable) cancellable = g_cancellable_new();
    g_autoptr(GError) error = nullptr;

    // Initialize AppStream store
    m_store = as_store_new();
    as_store_set_add_flags(m_store, (AsStoreAddFlags)(AS_STORE_ADD_FLAG_USE_UNIQUE_ID |
                                                      AS_STORE_ADD_FLAG_ONLY_NATIVE_LANGS));

    // Load flatpak installation
    if (!setupFlatpakInstallations(cancellable, &error)) {
        qWarning() << "Failed to setup flatpak installations: " << error->message;
    } else {
        reloadPackageList(cancellable);
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
    if (m_flatpakInstallationSystem) {
        g_object_unref(m_flatpakInstallationSystem);
    }
    if (m_flatpakInstallationUser) {
        g_object_unref(m_flatpakInstallationUser);
    }
    g_object_unref(m_store);
}

FlatpakRef * FlatpakBackend::createFakeRef(FlatpakResource *resource)
{
    FlatpakRef *ref = nullptr;
    g_autoptr(GError) localError = nullptr;

    const QString id = QString::fromUtf8("%1/%2/%3/%4").arg(resource->flatpakRefKind() == FLATPAK_REF_KIND_APP ? QLatin1String("app") : QLatin1String("runtime"))
                                                       .arg(resource->flatpakName()).arg(resource->arch()).arg(resource->branch());
    ref = flatpak_ref_parse(id.toStdString().c_str(), &localError);

    if (!ref) {
        qWarning() << "Failed to create fake ref: " << localError->message;
    }

    return ref;
}

FlatpakInstalledRef * FlatpakBackend::getInstalledRefForApp(FlatpakInstallation *flatpakInstallation, FlatpakResource *resource, GCancellable *cancellable)
{
    AsApp *app = resource->appstreamApp();
    AsAppKind appKind = as_app_get_kind(app);
    FlatpakInstalledRef *ref = nullptr;
    GPtrArray *installedApps = nullptr;
    g_autoptr(GError) localError = nullptr;

    if (!flatpakInstallation) {
        return ref;
    }

    ref = flatpak_installation_get_installed_ref(flatpakInstallation,
                                                 resource->flatpakRefKind(),
                                                 resource->flatpakName().toStdString().c_str(),
                                                 resource->arch().toStdString().c_str(),
                                                 resource->branch().toStdString().c_str(),
                                                 cancellable, &localError);

    // If we found installed ref this way, we can return it
    if (ref) {
        return ref;
    }

    // Otherwise go through all installed apps and try to match info we have
    installedApps = flatpak_installation_list_installed_refs_by_kind(flatpakInstallation,
                                                                     appKind == AS_APP_KIND_DESKTOP ? FLATPAK_REF_KIND_APP : FLATPAK_REF_KIND_RUNTIME,
                                                                     cancellable, &localError);
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

FlatpakResource * FlatpakBackend::getRuntimeForApp(FlatpakResource *resource)
{
    FlatpakResource *runtime = nullptr;
    const QStringList ids = m_resources.keys();
    const QStringList runtimeInfo = resource->runtime().split(QLatin1Char('/'));

    if (runtimeInfo.count() != 3) {
        return runtime;
    }

    const QString runtimeId = QString::fromUtf8("runtime/%1/%2").arg(runtimeInfo.at(0)).arg(runtimeInfo.at(2));
    foreach (const QString &id, ids) {
        if (id.endsWith(runtimeId)) {
            runtime = m_resources.value(id);
            break;
        }
    }

    // TODO if runtime wasn't found, create a new one from available info

    return runtime;
}

bool FlatpakBackend::compareAppFlatpakRef(FlatpakInstallation *flatpakInstallation, FlatpakResource *resource, FlatpakInstalledRef *ref)
{
    const QString arch = QString::fromUtf8(flatpak_ref_get_arch(FLATPAK_REF(ref)));
    const QString branch = QString::fromUtf8(flatpak_ref_get_branch(FLATPAK_REF(ref)));

    AsAppKind appKind = flatpak_ref_get_kind(FLATPAK_REF(ref)) == FLATPAK_REF_KIND_APP ? AS_APP_KIND_DESKTOP : AS_APP_KIND_RUNTIME;
    AsAppScope appScope = flatpak_installation_get_is_user(flatpakInstallation) ? AS_APP_SCOPE_USER : AS_APP_SCOPE_SYSTEM;
    g_autofree gchar *appId = nullptr;

    if (appKind == AS_APP_KIND_DESKTOP) {
        appId = g_strdup_printf("%s.desktop", flatpak_ref_get_name(FLATPAK_REF(ref)));
    } else {
        appId = g_strdup(flatpak_ref_get_name(FLATPAK_REF(ref)));
    }

    const QString uniqueId = QString::fromUtf8(as_utils_unique_id_build(appScope,
                                                                        AS_BUNDLE_KIND_FLATPAK,
                                                                        flatpak_installed_ref_get_origin(ref),
                                                                        appKind,
                                                                        appId,
                                                                        flatpak_ref_get_branch(FLATPAK_REF(ref))));

    // Compare uniqueId first then attempt to compare what we have
    if (resource->uniqueId() == uniqueId) {
        return true;
    }

    // Check if we have information about architecture and branch, otherwise compare names only
    // Happens with apps which don't have appstream metadata bug got here thanks to installed desktop file
    if (!resource->arch().isEmpty() && !resource->branch().isEmpty()) {
        return resource->arch() == arch && resource->branch() == branch && resource->flatpakName() == QString::fromUtf8(appId);
    }

    return resource->flatpakName() == QString::fromUtf8(appId);
}

bool FlatpakBackend::loadAppsFromAppstreamData(FlatpakInstallation *flatpakInstallation, GCancellable *cancellable)
{
    GPtrArray *apps;
    g_autoptr(GError) localError = nullptr;
    g_autoptr(GFile) appstreamDir = nullptr;
    g_autoptr(GPtrArray) remotes = nullptr;
    g_autoptr(AsStore) store = nullptr;
    g_autoptr(GFile) file = nullptr;

    if (!flatpakInstallation) {
        return false;
    }

    remotes = flatpak_installation_list_remotes(flatpakInstallation, cancellable, &localError);
    if (!remotes) {
        return false;
    }

    for (uint i = 0; i < remotes->len; i++) {
        FlatpakRemote *remote = FLATPAK_REMOTE(g_ptr_array_index(remotes, i));
        if (flatpak_remote_get_disabled(remote)) {
            continue;
        }

        appstreamDir = flatpak_remote_get_appstream_dir(remote, nullptr);
        if (!appstreamDir) {
            qWarning() << "No appstream dir for " << flatpak_remote_get_name(remote);
            continue;
        }

        QString appDirFileName = QString::fromUtf8(g_file_get_path(appstreamDir));
        appDirFileName += QLatin1String("/appstream.xml.gz");
        if (!QFile::exists(appDirFileName)) {
            qWarning() << "No " << appDirFileName << " appstream metadata found for " << flatpak_remote_get_name(remote);
            continue;
        }

        file = g_file_new_for_path(appDirFileName.toStdString().c_str());
        store = as_store_new();
        as_store_set_add_flags(store, (AsStoreAddFlags)(AS_STORE_ADD_FLAG_USE_UNIQUE_ID |
                                                        AS_STORE_ADD_FLAG_ONLY_NATIVE_LANGS));
        if (!as_store_from_file(store, file, nullptr, cancellable, &localError)) {
            qWarning() << "Failed to create AppStore from file";
            continue;
        }

        // Set icons path which we can use later for searching for icons and some other stuff
        apps = as_store_get_apps(store);
        for (uint i = 0; i < apps->len; i++) {
            AsApp *app = AS_APP(g_ptr_array_index(apps, i));

            // TODO Filter runtimes?
            // if (as_app_get_kind(app) != AS_APP_KIND_DESKTOP) {
            //     continue;
            // }

            as_app_set_icon_path(app, g_file_get_path(appstreamDir));
            if (flatpak_installation_get_is_user(flatpakInstallation)) {
                as_app_set_scope (app, AS_APP_SCOPE_USER);
            } else {
                as_app_set_scope (app, AS_APP_SCOPE_SYSTEM);
            }
            as_app_set_origin (app, flatpak_remote_get_name(remote));
            as_app_add_keyword (app, NULL, "flatpak");
        }

        as_store_load_search_cache(store);
        as_store_add_apps(m_store,  as_store_get_apps(store));
    }

    return true;
}

bool FlatpakBackend::loadInstalledApps(FlatpakInstallation *flatpakInstallation, GCancellable *cancellable)
{
    Q_UNUSED(cancellable);

    QDir dir;
    QString pathExports;
    QString pathApps;
    GPtrArray *icons;
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
            g_autoptr(GError) localError = nullptr;
            g_autoptr(AsApp) app = nullptr;

            if (file == QLatin1String("mimeinfo.cache")) {
                continue;
            }

            app = as_app_new();
            fnDesktop = pathApps + file;
            if (!as_app_parse_file(app, fnDesktop.toStdString().c_str(), (AsAppParseFlags) 0, &localError)) {
                qWarning() << "Failed to parse " << fnDesktop << localError->message;
                continue;
            }

            icons = as_app_get_icons(app);
            for (uint i = 0; i < icons->len; i++) {
                AsIcon *ic = AS_ICON(g_ptr_array_index(icons, i));
                if (as_icon_get_kind(ic) == AS_ICON_KIND_UNKNOWN) {
                    as_icon_set_kind(ic, AS_ICON_KIND_STOCK);
                    as_icon_set_prefix(ic, pathExports.toStdString().c_str());
                }
            }

            AsAppScope appScope = flatpak_installation_get_is_user(flatpakInstallation) ? AS_APP_SCOPE_USER : AS_APP_SCOPE_SYSTEM;
            as_app_add_keyword(app, nullptr, "flatpak");
            as_app_set_icon_path(app, pathExports.toStdString().c_str());
            as_app_set_kind(app, AS_APP_KIND_DESKTOP);
            as_app_set_state(app, AS_APP_STATE_INSTALLED);
            as_app_set_scope(app, appScope);
            as_app_set_source_file(app, fnDesktop.toStdString().c_str());

            as_store_add_app(m_store, app);
        }
    }

    return true;
}

bool FlatpakBackend::parseMetadataFromAppBundle(FlatpakResource *resource)
{
    g_autoptr(FlatpakRef) ref = nullptr;
    g_autoptr(GError) localError = nullptr;
    AsBundle *bundle = as_app_get_bundle_default(resource->appstreamApp());

    // Get arch/branch/commit/name from FlatpakRef
    if (bundle) {
        ref = flatpak_ref_parse(as_bundle_get_id(bundle), &localError);
        if (!ref) {
            qWarning() << "Failed to parse " << as_bundle_get_id(bundle) << localError->message;
            return false;
        } else {
            resource->setArch(QString::fromUtf8(flatpak_ref_get_arch(ref)));
            resource->setBranch(QString::fromUtf8(flatpak_ref_get_branch(ref)));
            resource->setFlatpakName(QString::fromUtf8(flatpak_ref_get_name(ref)));
            resource->setFlatpakRefKind(flatpak_ref_get_kind(ref));
        }
    }

    return true;
}

void FlatpakBackend::reloadPackageList(GCancellable *cancellable)
{
    GPtrArray *apps;

    // Load applications from appstream metadata
    if (!loadAppsFromAppstreamData(m_flatpakInstallationSystem, cancellable)) {
        qWarning() << "Failed to load packages from appstream data from system installation";
    }

    if (!loadAppsFromAppstreamData(m_flatpakInstallationUser, cancellable)) {
        qWarning() << "Failed to load packages from appstream data from user installation";
    }

    // Load installed applications and update existing resources with info from installed application
    if (!loadInstalledApps(m_flatpakInstallationSystem, cancellable)) {
        qWarning() << "Failed to load installed packages from system installation";
    }

    if (!loadInstalledApps(m_flatpakInstallationUser, cancellable)) {
        qWarning() << "Failed to load installed packages from user installation";
    }

    // Go through apps and runtimes
    apps = as_store_get_apps(m_store);
    for (uint i = 0; i < apps->len; i++) {
        AsApp *app = AS_APP(g_ptr_array_index(apps, i));

        // TODO think about adding runtimes and filter them later?
        // if (as_app_get_kind(app) == AS_APP_KIND_RUNTIME) {
        //    continue;
        // }

        // TODO maybe not to store AsApp internally and set every info in FlatpakResource properties
        FlatpakResource *resource = new FlatpakResource(app, this);

        AsAppState appState = as_app_get_state(app);
        AsAppScope appScope = as_app_get_scope(app);

        if (!parseMetadataFromAppBundle(resource)) {
            qWarning() << "Failed to parse metadata from app bundle for " << resource->name();
        }

        updateAppState(appScope == AS_APP_SCOPE_SYSTEM ? m_flatpakInstallationSystem : m_flatpakInstallationUser, resource, cancellable);

        if (resource->flatpakRefKind() == FLATPAK_REF_KIND_APP) {
            if (!updateAppMetadata(appScope == AS_APP_SCOPE_SYSTEM ? m_flatpakInstallationSystem : m_flatpakInstallationUser, resource, cancellable)) {
                qWarning() << "Failed to update " << resource->name() << " with installed metadata";
            }
        }

        updateAppSize(appScope == AS_APP_SCOPE_SYSTEM ? m_flatpakInstallationSystem : m_flatpakInstallationUser, resource, cancellable);

        m_resources.insert(resource->uniqueId(), resource);
    }
}

bool FlatpakBackend::setupFlatpakInstallations(GCancellable *cancellable, GError **error)
{
    m_flatpakInstallationSystem = flatpak_installation_new_system(cancellable, error);
    if (!m_flatpakInstallationSystem) {
        return false;
    }

    m_flatpakInstallationUser = flatpak_installation_new_user(cancellable, error);
    if (!m_flatpakInstallationUser) {
        return false;
    }

    return true;
}

void FlatpakBackend::updateAppInstalledMetadata(FlatpakInstalledRef *installedRef, FlatpakResource *resource)
{
    AsApp *app = resource->appstreamApp();

    as_app_set_branch(app, flatpak_ref_get_branch(FLATPAK_REF(installedRef)));
    as_app_set_origin(app, flatpak_installed_ref_get_origin(installedRef));

    // Update the rest
    resource->setArch(QString::fromUtf8(flatpak_ref_get_arch(FLATPAK_REF(installedRef))));
    resource->setCommit(QString::fromUtf8(flatpak_ref_get_commit(FLATPAK_REF(installedRef))));
    resource->setFlatpakName(QString::fromUtf8(flatpak_ref_get_name(FLATPAK_REF(installedRef))));
    resource->setSize(flatpak_installed_ref_get_installed_size(installedRef));
    resource->setState(AbstractResource::Installed);
}

bool FlatpakBackend::updateAppMetadata(FlatpakInstallation* flatpakInstallation, FlatpakResource *resource, GCancellable *cancellable)
{
    QString metadataContent;
    QString path;
    gsize len = 0;
    g_autoptr(GBytes) data = nullptr;
    g_autoptr(GFile) installationPath = nullptr;
    g_autoptr(GError) localError = nullptr;

    if (as_app_get_kind(resource->appstreamApp()) == AS_APP_KIND_SOURCE) {
        return true;
    }

    if (resource->flatpakRefKind() != FLATPAK_REF_KIND_APP) {
        return true;
    }

    installationPath = flatpak_installation_get_path(flatpakInstallation);
    path = QString::fromUtf8(g_file_get_path(installationPath));
    path += QString::fromUtf8("/app/%1/%2/%3/active/metadata").arg(resource->flatpakName()).arg(resource->arch()).arg(resource->branch());

    if (QFile::exists(path)) {
        QFile file(path);
        if (file.open(QFile::ReadOnly | QFile::Text)) {
            QTextStream stream(&file);
            metadataContent = stream.readAll();
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

        data = flatpak_installation_fetch_remote_metadata_sync(flatpakInstallation, resource->origin().toStdString().c_str(), fakeRef, cancellable, &localError);
        if (data) {
            metadataContent = QString::fromUtf8((char *)g_bytes_get_data(data, &len));
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

    tempFile.write(metadataContent.toUtf8());
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

bool FlatpakBackend::updateAppSize(FlatpakInstallation *flatpakInstallation, FlatpakResource *resource, GCancellable *cancellable)
{
    int size;
    guint64 downloadSize = 0;
    guint64 installedSize = 0;

    // TODO take runtimes and related apps into account

    if (as_app_get_state(resource->appstreamApp()) == AS_APP_STATE_AVAILABLE_LOCAL) {
        return true;
    }

    if (as_app_get_kind(resource->appstreamApp()) == AS_APP_KIND_SOURCE) {
        return true;
    }

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
        if (!updateAppMetadata(flatpakInstallation, resource, cancellable)) {
            qWarning() << "Failed to get runtime for " << resource->name() << " needed for calculating of size";
            return false;
        }
    }

    // Calculate the runtime size
    FlatpakResource *runtime = nullptr;
    if (resource->state() == AbstractResource::None && resource->flatpakRefKind() == FLATPAK_REF_KIND_APP) {
        runtime = getRuntimeForApp(resource);
        if (runtime) {
            // Re-check runtime state if case a new one was created
            updateAppState(flatpakInstallation, runtime, cancellable);

            if (!runtime->isInstalled()) {
                if (!updateAppSize(flatpakInstallation, runtime, cancellable)) {
                    qWarning() << "Failed to get runtime size needed for total size of " << resource->name();
                    return false;
                }
            }
        }
    }

    if (resource->isInstalled()) {
        g_autoptr(FlatpakInstalledRef) ref = nullptr;
        ref = getInstalledRefForApp(flatpakInstallation, resource, cancellable);
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
                                                         ref, &downloadSize, &installedSize, cancellable, &localError)) {
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

void FlatpakBackend::updateAppState(FlatpakInstallation *flatpakInstallation, FlatpakResource *resource, GCancellable *cancellable)
{
    FlatpakInstalledRef *ref = getInstalledRefForApp(flatpakInstallation, resource, cancellable);
    if (ref) {
        // If the app is installed, we can set information about commit, arch etc.
        updateAppInstalledMetadata(ref, resource);
    } else {
        // TODO check if the app is actuall still available
        resource->setState(AbstractResource::None);
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
        if (qobject_cast<FlatpakResource*>(r)->flatpakRefKind() != FLATPAK_REF_KIND_APP) {
            continue;
        }

        if(r->name().contains(filter.search, Qt::CaseInsensitive) || r->comment().contains(filter.search, Qt::CaseInsensitive))
            ret += r;
    }
    return new ResultsStream(QStringLiteral("FlatpakStream"), ret);
}

ResultsStream * FlatpakBackend::findResourceByPackageName(const QUrl &search)
{
    auto res = search.scheme() == QLatin1String("flatpak") ? m_resources.value(search.host().replace(QLatin1Char('.'), QLatin1Char(' '))) : nullptr;
    if (!res) {
        return new ResultsStream(QStringLiteral("FlatpakStream"), {});
    } else {
        return new ResultsStream(QStringLiteral("FlatpakStream"), { res });
    }
}

AbstractBackendUpdater * FlatpakBackend::backendUpdater() const
{
    return m_updater;
}

AbstractReviewsBackend * FlatpakBackend::reviewsBackend() const
{
    return m_reviews;
}

FlatpakInstallation * FlatpakBackend::flatpakInstallationForAppScope(AsAppScope appScope) const
{
    if (appScope == AS_APP_SCOPE_SYSTEM) {
        return m_flatpakInstallationSystem;
    } else {
        return m_flatpakInstallationUser;
    }
}

void FlatpakBackend::installApplication(AbstractResource *app, const AddonList &addons)
{
    FlatpakResource *resource = qobject_cast<FlatpakResource*>(app);
    FlatpakInstallation *installation = as_app_get_scope(resource->appstreamApp()) == AS_APP_SCOPE_SYSTEM ? m_flatpakInstallationSystem : m_flatpakInstallationUser;

    // TODO: Check if the runtime needed by the application is installed

    FlatpakTransaction *transaction = new FlatpakTransaction(installation, resource, addons, Transaction::InstallRole);
    connect(transaction, &FlatpakTransaction::statusChanged, [this, installation, resource] (Transaction::Status status) {
        if (status == Transaction::Status::DoneStatus) {
            updateAppState(installation, resource, nullptr);
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
    FlatpakInstallation *installation = as_app_get_scope(resource->appstreamApp()) == AS_APP_SCOPE_SYSTEM ? m_flatpakInstallationSystem : m_flatpakInstallationUser;
    FlatpakTransaction *transaction = new FlatpakTransaction(installation, resource, Transaction::RemoveRole);
}

void FlatpakBackend::checkForUpdates()
{
//     if(m_fetching)
//         return;
//     toggleFetching();
//     populate(QStringLiteral("Moar"));
//     QTimer::singleShot(500, this, &FlatpakBackend::toggleFetching);
}

// AbstractResource * FlatpakBackend::resourceForFile(const QUrl &path)
// {
//     FlatpakResource* res = new FlatpakResource(path.fileName(), true, this);
//     res->setSize(666);
//     res->setState(AbstractResource::None);
//     m_resources.insert(res->packageName(), res);
//     connect(res, &FlatpakResource::stateChanged, this, &FlatpakBackend::updatesCountChanged);
//     return res;
// }

#include "FlatpakBackend.moc"
