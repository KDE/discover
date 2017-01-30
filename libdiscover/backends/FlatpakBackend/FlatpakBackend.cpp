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
#include <QThread>
#include <QTimer>

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

    // Load flatpak installation
    if (!setupFlatpakInstallations(cancellable, &error)) {
        qWarning() << "Failed to setup flatpak installations: " << error->message;
    } else {
        reloadPackageList(cancellable, &error);
    }

    QAction* updateAction = new QAction(this);
    updateAction->setIcon(QIcon::fromTheme(QStringLiteral("system-software-update")));
    updateAction->setText(i18nc("@action Checks the Internet for updates", "Check for Updates"));
    updateAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_R));
    connect(updateAction, &QAction::triggered, this, &FlatpakBackend::checkForUpdates);

    m_messageActions = QList<QAction*>() << updateAction;

    SourcesModel::global()->addSourcesBackend(new FlatpakSourcesBackend(this));
}

bool FlatpakBackend::loadAppsFromAppstreamData(FlatpakInstallation *flatpakInstallation, GCancellable *cancellable, GError **error)
{
    GPtrArray *apps;
    g_autoptr(GFile) appstreamDir = nullptr;
    g_autoptr(GPtrArray) remotes = nullptr;
    g_autoptr(AsStore) store = nullptr;
    g_autoptr(GFile) file = nullptr;

    if (!flatpakInstallation) {
        return false;
    }

    remotes = flatpak_installation_list_remotes(flatpakInstallation, cancellable, error);
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
        if (!as_store_from_file(store, file, nullptr, cancellable, error)) {
            qWarning() << "Failed to create AppStore from file";
            continue;
        }

        // Set icons path which we can use later for searching for icons and some other stuff
        apps = as_store_get_apps(store);
        for (uint i = 0; i < apps->len; i++) {
            AsApp *app = AS_APP(g_ptr_array_index(apps, i));
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

bool FlatpakBackend::loadInstalledApps(FlatpakInstallation *flatpakInstallation, GCancellable *cancellable, GError **error)
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

bool FlatpakBackend::updateAppWithInstalledMetadata(FlatpakInstallation *flatpakInstallation, FlatpakResource *resource, GCancellable *cancellable, GError **error)
{
    AsApp *app = resource->appstreamApp();
    g_autoptr(GPtrArray) installedApps = nullptr;

    if (!flatpakInstallation) {
        return false;
    }

    // List installed applications from flatpak installation
    installedApps = flatpak_installation_list_installed_refs(flatpakInstallation, cancellable, error);
    if (!installedApps) {
        return false;
    }

    for (uint i = 0; i < installedApps->len; i++) {
        FlatpakInstalledRef *installedRef = FLATPAK_INSTALLED_REF(g_ptr_array_index(installedApps, i));

        // We want to list only applications
        if (flatpak_ref_get_kind(FLATPAK_REF(installedRef)) != FLATPAK_REF_KIND_APP) {
            continue;
        }

        // Only show current applications
        if (!flatpak_installed_ref_get_is_current(installedRef)) {
            continue;
        }

        // Check if the installed_reference and app_id are the same and update the app with installed metadata
        if (g_strcmp0(as_app_get_id(app), g_strdup_printf("%s.desktop" ,flatpak_ref_get_name(FLATPAK_REF(installedRef)))) == 0) {
            // Update appstream metadata
            as_app_set_branch(app, flatpak_ref_get_branch(FLATPAK_REF(installedRef)));
            as_app_set_origin(app, flatpak_installed_ref_get_origin(installedRef));

            // Update the rest
            resource->setArch(QString::fromUtf8(flatpak_ref_get_arch(FLATPAK_REF(installedRef))));
            resource->setCommit(QString::fromUtf8(flatpak_ref_get_commit(FLATPAK_REF(installedRef))));
            resource->setFlatpakName(QString::fromUtf8(flatpak_ref_get_name(FLATPAK_REF(installedRef))));
            resource->setSize(flatpak_installed_ref_get_installed_size(installedRef));

            return true;
        }
    }

    g_set_error(error, FLATPAK_ERROR, FLATPAK_ERROR_NOT_INSTALLED, "Couldn't find installed application");
    return false;
}

void FlatpakBackend::reloadPackageList(GCancellable *cancellable, GError **error)
{
    GPtrArray *apps;
    g_autoptr(GError) localError = nullptr;

    // Load applications from appstream metadata
    if (!loadAppsFromAppstreamData(m_flatpakInstallationSystem, cancellable, &localError)) {
        qWarning() << "Failed to load packages from appstream data from system installation";
    }

    if (!loadAppsFromAppstreamData(m_flatpakInstallationUser, cancellable, &localError)) {
        qWarning() << "Failed to load packages from appstream data from user installation";
    }

    // Load installed applications and update existing resources with info from installed application
    if (!loadInstalledApps(m_flatpakInstallationSystem, cancellable, &localError)) {
        qWarning() << "Failed to load installed packages from system installation";
    }

    if (!loadInstalledApps(m_flatpakInstallationUser, cancellable, &localError)) {
        qWarning() << "Failed to load installed packages from user installation";
    }

    apps = as_store_get_apps(m_store);
    for (uint i = 0; i < apps->len; i++) {
        AsApp *app = AS_APP(g_ptr_array_index(apps, i));

        if (as_app_get_kind(app) == AS_APP_KIND_RUNTIME) {
            continue;
        }

        FlatpakResource  *resource = new FlatpakResource(app, this);

        // Update application with installed metadata
        AsAppState appState = as_app_get_state(app);
        if (appState == AS_APP_STATE_INSTALLED || appState == AS_APP_STATE_UPDATABLE || appState == AS_APP_STATE_UPDATABLE_LIVE) {
            AsAppScope appScope = as_app_get_scope(app);
            if (appScope == AS_APP_SCOPE_SYSTEM) {
                if (!updateAppWithInstalledMetadata(m_flatpakInstallationSystem, resource, cancellable, &localError)) {
                    qWarning() << "Failed to update " << as_app_get_name(app, nullptr) << " with installed metadata";
                }
            } else {
                if (!updateAppWithInstalledMetadata(m_flatpakInstallationUser, resource, cancellable, &localError)) {
                    qWarning() << "Failed to update " << as_app_get_name(app, nullptr) << " with installed metadata";
                }
            }
        }

        m_resources.insert(resource->uniqueId(), resource);
    }
}

int FlatpakBackend::updatesCount() const
{
    return m_updater->updatesCount();
}

ResultsStream* FlatpakBackend::search(const AbstractResourcesBackend::Filters &filter)
{
    QVector<AbstractResource*> ret;
    foreach(AbstractResource* r, m_resources) {
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
    } else
        return new ResultsStream(QStringLiteral("FlatpakStream"), { res });
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

void FlatpakBackend::installApplication(AbstractResource* app, const AddonList &addons)
{
    TransactionModel *transModel = TransactionModel::global();
    transModel->addTransaction(new FlatpakTransaction(qobject_cast<FlatpakResource*>(app), addons, Transaction::InstallRole));
}

void FlatpakBackend::installApplication(AbstractResource* app)
{
    TransactionModel *transModel = TransactionModel::global();
    transModel->addTransaction(new FlatpakTransaction(qobject_cast<FlatpakResource*>(app), Transaction::InstallRole));
}

void FlatpakBackend::removeApplication(AbstractResource* app)
{
    TransactionModel *transModel = TransactionModel::global();
    transModel->addTransaction(new FlatpakTransaction(qobject_cast<FlatpakResource*>(app), Transaction::RemoveRole));
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
