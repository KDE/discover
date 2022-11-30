/*
 *   SPDX-FileCopyrightText: 2013 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *   SPDX-FileCopyrightText: 2017 Jan Grulich <jgrulich@redhat.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "FlatpakResource.h"
#include "FlatpakBackend.h"
#include "FlatpakFetchDataJob.h"
#include "FlatpakSourcesBackend.h"
#include "config-paths.h"

#include <Transaction/AddonList.h>

#include <AppStreamQt/icon.h>
#include <AppStreamQt/screenshot.h>
#include <AppStreamQt/utils.h>
#include <appstream/AppStreamUtils.h>

#include <KConfigGroup>
#include <KDesktopFile>
#include <KFormat>
#include <KIO/ApplicationLauncherJob>
#include <KLocalizedString>

#include <AppStreamQt/release.h>
#include <QDebug>
#include <QDesktopServices>
#include <QDir>
#include <QDirIterator>
#include <QFileInfo>
#include <QFutureWatcher>
#include <QIcon>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QProcess>
#include <QStringList>
#include <QTemporaryFile>
#include <QTimer>
#include <QUrlQuery>
#include <QtConcurrentRun>

static QString iconCachePath(const AppStream::Icon &icon)
{
    Q_ASSERT(icon.kind() == AppStream::Icon::KindRemote);
    return QStringLiteral("%1/icons/%2").arg(QStandardPaths::writableLocation(QStandardPaths::CacheLocation), icon.url().fileName());
}

const QStringList FlatpakResource::s_objects({
    QStringLiteral("qrc:/qml/FlatpakAttention.qml"),
    QStringLiteral("qrc:/qml/FlatpakRemoveData.qml"),
    QStringLiteral("qrc:/qml/FlatpakOldBeta.qml"),
    QStringLiteral("qrc:/qml/FlatpakEolReason.qml"),
});
const QStringList FlatpakResource::s_bottomObjects({QStringLiteral("qrc:/qml/PermissionsList.qml")});
Q_GLOBAL_STATIC(QNetworkAccessManager, manager)

FlatpakResource::FlatpakResource(const AppStream::Component &component, FlatpakInstallation *installation, FlatpakBackend *parent)
    : AbstractResource(parent)
    , m_appdata(component)
    , m_id({component.id(), QString(), QString()})
    , m_downloadSize(0)
    , m_installedSize(0)
    , m_propertyStates({{DownloadSize, NotKnownYet}, {InstalledSize, NotKnownYet}, {RequiredRuntime, NotKnownYet}})
    , m_state(AbstractResource::None)
    , m_installation(installation)
{
    setObjectName(packageName());

    // Start fetching remote icons during initialization
    const auto icons = m_appdata.icons();
    if (icons.count() == 1 && icons.constFirst().kind() == AppStream::Icon::KindRemote) {
        const auto icon = icons.constFirst();
        const QString fileName = iconCachePath(icon);
        if (!QFileInfo::exists(fileName)) {
            const QDir cacheDir(QStandardPaths::writableLocation(QStandardPaths::CacheLocation));
            // Create $HOME/.cache/discover/icons folder
            cacheDir.mkdir(QStringLiteral("icons"));
            auto reply = manager->get(QNetworkRequest(icon.url()));
            connect(reply, &QNetworkReply::finished, this, [this, icon, fileName, reply] {
                if (reply->error() == QNetworkReply::NoError) {
                    QByteArray iconData = reply->readAll();
                    QFile file(fileName);
                    if (file.open(QIODevice::WriteOnly)) {
                        file.write(iconData);
                    } else {
                        qDebug() << "could not find icon for" << packageName() << reply->url();
                        QIcon::fromTheme(QStringLiteral("package-x-generic")).pixmap(32, 32).toImage().save(fileName);
                    }
                    file.close();
                    Q_EMIT iconChanged();
                    reply->deleteLater();
                }
            });
        }
    }

    connect(this, &FlatpakResource::stateChanged, this, &FlatpakResource::hasDataChanged);
}

AppStream::Component FlatpakResource::appstreamComponent() const
{
    return m_appdata;
}

QList<PackageState> FlatpakResource::addonsInformation()
{
    return {};
}

QString FlatpakResource::availableVersion() const
{
    if (m_availableVersion.isEmpty()) {
        const auto releases = m_appdata.releases();
        if (!releases.isEmpty()) {
            m_availableVersion = releases.constFirst().version();
            return m_availableVersion;
        }
    } else {
        return m_availableVersion;
    }
    return branch();
}

QString FlatpakResource::appstreamId() const
{
    return m_id.id;
}

QString FlatpakResource::arch() const
{
    return m_id.arch;
}

QString FlatpakResource::branch() const
{
    return m_id.branch;
}

bool FlatpakResource::canExecute() const
{
    return (m_type == DesktopApp && (m_state == AbstractResource::Installed || m_state == AbstractResource::Upgradeable));
}

void FlatpakResource::updateFromRef(FlatpakRef *ref)
{
    setArch(QString::fromUtf8(flatpak_ref_get_arch(ref)));
    setBranch(QString::fromUtf8(flatpak_ref_get_branch(ref)));
    setCommit(QString::fromUtf8(flatpak_ref_get_commit(ref)));
    setFlatpakName(QString::fromUtf8(flatpak_ref_get_name(ref)));
    setType(flatpak_ref_get_kind(ref) == FLATPAK_REF_KIND_APP ? DesktopApp : extends().isEmpty() ? Runtime : Extension);
    setObjectName(packageName());
}

void FlatpakResource::updateFromAppStream()
{
    const QString refstr = m_appdata.bundle(AppStream::Bundle::KindFlatpak).id();
    g_autoptr(GError) localError = nullptr;
    g_autoptr(FlatpakRef) ref = flatpak_ref_parse(refstr.toUtf8().constData(), &localError);
    if (!ref) {
        qDebug() << "failed to obtain ref" << refstr << localError->message;
        return;
    }
    updateFromRef(ref);
}

QString FlatpakResource::ref() const
{
    return typeAsString() + QLatin1Char('/') + flatpakName() + QLatin1Char('/') + arch() + QLatin1Char('/') + branch();
}

QStringList FlatpakResource::categories()
{
    auto cats = m_appdata.categories();
    if (m_appdata.kind() != AppStream::Component::KindAddon)
        cats.append(QStringLiteral("Application"));
    return cats;
}

QString FlatpakResource::comment()
{
    const auto summary = m_appdata.summary();
    if (!summary.isEmpty()) {
        return summary;
    }

    return QString();
}

QString FlatpakResource::commit() const
{
    return m_commit;
}

quint64 FlatpakResource::downloadSize() const
{
    return m_downloadSize;
}

QVariant FlatpakResource::icon() const
{
    QIcon ret;
    const auto icons = m_appdata.icons();

    if (!m_bundledIcon.isNull()) {
        ret = QIcon(m_bundledIcon);
    } else if (icons.isEmpty()) {
        ret = QIcon::fromTheme(QStringLiteral("package-x-generic"));
    } else
        for (const AppStream::Icon &icon : icons) {
            switch (icon.kind()) {
            case AppStream::Icon::KindLocal:
            case AppStream::Icon::KindCached: {
                const QString path = icon.url().toLocalFile();
                if (QDir::isRelativePath(path)) {
                    const QString appstreamLocation = installationPath() + "/appstream/" + origin() + '/' + flatpak_get_default_arch() + "/active/icons/";
                    QDirIterator dit(appstreamLocation, QDirIterator::Subdirectories);
                    while (dit.hasNext()) {
                        const auto currentPath = dit.next();
                        if (dit.fileName() == path) {
                            ret.addFile(currentPath, icon.size());
                        }
                    }
                } else {
                    ret.addFile(path, icon.size());
                }
            } break;
            case AppStream::Icon::KindStock: {
                const auto ret = QIcon::fromTheme(icon.name());
                if (!ret.isNull())
                    return ret;
                break;
            }
            case AppStream::Icon::KindRemote: {
                const QString fileName = iconCachePath(icon);
                if (QFileInfo::exists(fileName)) {
                    ret.addFile(fileName, icon.size());
                }
                break;
            }
            case AppStream::Icon::KindUnknown:
                break;
            }
        }

    if (ret.isNull()) {
        ret = QIcon::fromTheme(QStringLiteral("package-x-generic"));
    }

    return ret;
}

QString FlatpakResource::installedVersion() const
{
    g_autoptr(FlatpakInstalledRef) ref = qobject_cast<FlatpakBackend *>(backend())->getInstalledRefForApp(this);
    if (ref) {
        const char *appdataVersion = flatpak_installed_ref_get_appdata_version(ref);

        if (appdataVersion) {
            return appdataVersion;
        }
    }
    return branch();
}

quint64 FlatpakResource::installedSize() const
{
    return m_installedSize;
}

AbstractResource::Type FlatpakResource::type() const
{
    switch (m_type) {
    case FlatpakResource::Runtime:
        return Technical;
    case FlatpakResource::Extension:
        return Addon;
    default:
        return Application;
    }
}

QUrl FlatpakResource::homepage()
{
    return m_appdata.url(AppStream::Component::UrlKindHomepage);
}

QUrl FlatpakResource::helpURL()
{
    return m_appdata.url(AppStream::Component::UrlKindHelp);
}

QUrl FlatpakResource::bugURL()
{
    return m_appdata.url(AppStream::Component::UrlKindBugtracker);
}

QUrl FlatpakResource::donationURL()
{
    return m_appdata.url(AppStream::Component::UrlKindDonation);
}

QUrl FlatpakResource::contributeURL()
{
    return m_appdata.url(AppStream::Component::UrlKindContribute);
}

FlatpakResource::FlatpakFileType FlatpakResource::flatpakFileType() const
{
    return m_flatpakFileType;
}

QString FlatpakResource::flatpakName() const
{
    // If the flatpak name is not known (known only for installed apps), then use
    // appstream id instead;
    if (m_flatpakName.isEmpty()) {
        return m_id.id;
    }

    return m_flatpakName;
}

QJsonArray FlatpakResource::licenses()
{
    return AppStreamUtils::licenses(m_appdata);
}

QString FlatpakResource::longDescription()
{
    return m_appdata.description();
}

QString FlatpakResource::attentionText() const
{
    if (m_flatpakFileType == FlatpakResource::FileFlatpakRef) {
        QUrl loc = m_resourceLocation;
        loc.setPath({});
        loc.setQuery(QUrlQuery());
        return xi18nc("@info",
                      "<para>This application comes from \"%1\" (hosted at <link>%2</link>). Other software in this repository will also be made be available "
                      "in Discover "
                      "when the application is "
                      "installed.</para>",
                      m_origin,
                      loc.toDisplayString());
    }
    return {};
}

QAbstractListModel *FlatpakResource::permissionsModel()
{
    if (m_permissions.empty()) {
        loadPermissions();
    }
    return new FlatpakPermissionsModel(m_permissions);
}

QString FlatpakResource::name() const
{
    QString name = m_appdata.name();
    if (name.isEmpty()) {
        name = flatpakName();
    }

    if (name.startsWith(QLatin1String("(Nightly) "))) {
        return name.mid(10);
    }

    return name;
}

QString FlatpakResource::origin() const
{
    return m_origin;
}

QString FlatpakResource::displayOrigin() const
{
    return !m_displayOrigin.isEmpty() ? m_displayOrigin : m_origin;
}

QString FlatpakResource::packageName() const
{
    return flatpakName() + QLatin1Char('/') + arch() + QLatin1Char('/') + branch();
}

FlatpakResource::PropertyState FlatpakResource::propertyState(FlatpakResource::PropertyKind kind) const
{
    return m_propertyStates[kind];
}

QUrl FlatpakResource::resourceFile() const
{
    return m_resourceFile;
}

QString FlatpakResource::runtime() const
{
    return m_runtime;
}

QString FlatpakResource::section()
{
    return QString();
}

quint64 FlatpakResource::size()
{
    if (m_state == Installed) {
        return m_installedSize;
    } else {
        return m_downloadSize;
    }
}

QString FlatpakResource::sizeDescription()
{
    KFormat f;
    if (!isInstalled() || canUpgrade()) {
        if (propertyState(DownloadSize) == NotKnownYet || propertyState(InstalledSize) == NotKnownYet || propertyState(DownloadSize) == Fetching
            || propertyState(InstalledSize) == Fetching) {
            qobject_cast<FlatpakBackend *>(backend())->updateAppSize(this);
            return i18n("Retrieving size information");
        } else if (propertyState(DownloadSize) == UnknownOrFailed || propertyState(InstalledSize) == UnknownOrFailed) {
            return i18n("Unknown size");
        } else {
            return i18nc("@info app size", "%1 to download, %2 on disk", f.formatByteSize(downloadSize()), f.formatByteSize(installedSize()));
        }
    } else {
        if (propertyState(InstalledSize) == NotKnownYet || propertyState(InstalledSize) == Fetching) {
            return i18n("Retrieving size information");
        } else if (propertyState(InstalledSize) == UnknownOrFailed) {
            return i18n("Unknown size");
        } else {
            return i18nc("@info app size", "%1 on disk", f.formatByteSize(installedSize()));
        }
    }
}

AbstractResource::State FlatpakResource::state()
{
    return m_state;
}

FlatpakResource::ResourceType FlatpakResource::resourceType() const
{
    return m_type;
}

QString FlatpakResource::typeAsString() const
{
    switch (m_type) {
    case FlatpakResource::Runtime:
    case FlatpakResource::Extension:
        return QLatin1String("runtime");
    case FlatpakResource::DesktopApp:
    case FlatpakResource::Source:
    default:
        return QLatin1String("app");
    }
}

FlatpakResource::Id FlatpakResource::uniqueId() const
{
    return m_id;
}

void FlatpakResource::invokeApplication() const
{
    QString desktopFileName;
    auto launchables = m_appdata.launchable(AppStream::Launchable::KindDesktopId).entries();
    if (!launchables.isEmpty()) {
        desktopFileName = launchables.constFirst();
    } else {
        qWarning() << "Failed to find launchable for " << m_appdata.name() << ", using AppStream identifier instead";
        desktopFileName = appstreamId();
    }

    KService::Ptr service = KService::serviceByStorageId(desktopFileName);

    if (!service) {
        qWarning() << "Failed to find service" << desktopFileName;
        return;
    }

    auto *job = new KIO::ApplicationLauncherJob(service);
    connect(job, &KJob::finished, this, [this, service](KJob *job) {
        if (job->error()) {
            Q_EMIT backend()->passiveMessage(i18n("Failed to start '%1': %2", service->name(), job->errorString()));
        }
    });

    job->start();
}

void FlatpakResource::fetchChangelog()
{
    Q_EMIT changelogFetched(AppStreamUtils::changelogToHtml(m_appdata));
}

void FlatpakResource::fetchScreenshots()
{
    const auto sc = AppStreamUtils::fetchScreenshots(m_appdata);
    Q_EMIT screenshotsFetched(sc.first, sc.second);
}

void FlatpakResource::setArch(const QString &arch)
{
    m_id.arch = arch;
}

void FlatpakResource::setBranch(const QString &branch)
{
    m_id.branch = branch;
}

void FlatpakResource::setBundledIcon(const QPixmap &pixmap)
{
    m_bundledIcon = pixmap;
}

void FlatpakResource::setCommit(const QString &commit)
{
    m_commit = commit;
}

void FlatpakResource::setDownloadSize(quint64 size)
{
    m_downloadSize = size;

    setPropertyState(DownloadSize, AlreadyKnown);

    Q_EMIT sizeChanged();
}

void FlatpakResource::setFlatpakFileType(FlatpakFileType fileType)
{
    m_flatpakFileType = fileType;
}

void FlatpakResource::setFlatpakName(const QString &name)
{
    m_flatpakName = name;
}

void FlatpakResource::setIconPath(const QString &path)
{
    m_iconPath = path;
}

void FlatpakResource::setInstalledSize(quint64 size)
{
    m_installedSize = size;

    setPropertyState(InstalledSize, AlreadyKnown);

    Q_EMIT sizeChanged();
}

void FlatpakResource::setOrigin(const QString &origin)
{
    m_origin = origin;
}

void FlatpakResource::setDisplayOrigin(const QString &displayOrigin)
{
    m_displayOrigin = displayOrigin;
}

void FlatpakResource::setPropertyState(FlatpakResource::PropertyKind kind, FlatpakResource::PropertyState newState)
{
    auto &state = m_propertyStates[kind];
    if (state != newState) {
        state = newState;

        Q_EMIT propertyStateChanged(kind, newState);
    }
}

void FlatpakResource::setResourceFile(const QUrl &url)
{
    m_resourceFile = url;
}

void FlatpakResource::setRuntime(const QString &runtime)
{
    m_runtime = runtime;

    setPropertyState(RequiredRuntime, AlreadyKnown);
}

void FlatpakResource::setState(AbstractResource::State state, bool shouldEmit)
{
    if (m_state != state) {
        m_state = state;

        if (shouldEmit && qobject_cast<FlatpakBackend *>(backend())->isTracked(this)) {
            Q_EMIT stateChanged();
        }
    }
}

void FlatpakResource::setType(FlatpakResource::ResourceType type)
{
    m_type = type;
}

QString FlatpakResource::installationPath() const
{
    return installationPath(m_installation);
}

QString FlatpakResource::installationPath(FlatpakInstallation *flatpakInstallation)
{
    g_autoptr(GFile) path = flatpak_installation_get_path(flatpakInstallation);
    g_autofree char *path_str = g_file_get_path(path);
    return QString::fromUtf8(path_str);
}

QString FlatpakResource::installPath() const
{
    return installationPath() + QStringLiteral("/app/%1/%2/%3/active").arg(flatpakName(), arch(), branch());
}

QUrl FlatpakResource::url() const
{
    if (!m_resourceFile.isEmpty()) {
        return m_resourceFile;
    }

    QUrl ret(QStringLiteral("appstream://") + appstreamId());
    const AppStream::Provided::Kind AppStream_Provided_KindId = (AppStream::Provided::Kind)12; // Should be AppStream::Provided::KindId when released
    const auto provided = m_appdata.provided(AppStream_Provided_KindId).items();
    if (!provided.isEmpty()) {
        QUrlQuery qq;
        qq.addQueryItem("alt", provided.join(QLatin1Char(',')));
        ret.setQuery(qq);
    }
    return ret;
}

QDate FlatpakResource::releaseDate() const
{
    if (!m_appdata.releases().isEmpty()) {
        auto release = m_appdata.releases().constFirst();
        return release.timestamp().date();
    }

    return {};
}

QString FlatpakResource::sourceIcon() const
{
    const auto sourceItem = qobject_cast<FlatpakBackend *>(backend())->sources()->sourceById(origin());
    if (!sourceItem) {
        qWarning() << "Could not find source " << origin();
        return QStringLiteral("flatpak-discover");
    }

    const auto iconUrl = sourceItem->data(FlatpakSourcesBackend::IconUrlRole).toString();
    if (iconUrl.isEmpty())
        return QStringLiteral("flatpak-discover");
    return iconUrl;
}

QString FlatpakResource::author() const
{
    QString name = m_appdata.developerName();

    if (name.isEmpty()) {
        name = m_appdata.projectGroup();
    }

    return name;
}

QStringList FlatpakResource::extends() const
{
    return m_appdata.extends();
}

QSet<QString> FlatpakResource::alternativeAppstreamIds() const
{
    const AppStream::Provided::Kind AppStream_Provided_KindId = (AppStream::Provided::Kind)12; // Should be AppStream::Provided::KindId when released
    const auto ret = m_appdata.provided(AppStream_Provided_KindId).items();

    return QSet<QString>(ret.begin(), ret.end());
}

QStringList FlatpakResource::mimetypes() const
{
    return m_appdata.provided(AppStream::Provided::KindMimetype).items();
}

QString FlatpakResource::versionString()
{
    QString version;
    if (resourceType() == Source) {
        return {};
    }
    if (isInstalled()) {
        auto ref = qobject_cast<FlatpakBackend *>(backend())->getInstalledRefForApp(this);
        if (ref) {
            version = flatpak_installed_ref_get_appdata_version(ref);
        }
    } else if (!m_appdata.releases().isEmpty()) {
        auto release = m_appdata.releases().constFirst();
        version = release.version();
    } else {
        version = m_id.branch;
    }

    return AppStreamUtils::versionString(version, m_appdata);
}

QString translateSymbolicName(const QStringView &name)
{
    if (name == QLatin1String("host")) {
        return i18n("All Files");
    } else if (name == QLatin1String("home")) {
        return i18n("Home");
    } else if (name == QLatin1String("xdg-download")) {
        return i18n("Downloads");
    } else if (name == QLatin1String("xdg-music")) {
        return i18n("Music");
    }
    return name.toString();
}

QString FlatpakResource::eolReason()
{
    if (!m_eolReason.has_value()) {
        auto futureWatcher = new QFutureWatcher<FlatpakRemoteRef *>(this);
        connect(futureWatcher, &QFutureWatcher<FlatpakRemoteRef *>::finished, this, [this, futureWatcher]() {
            g_autoptr(FlatpakRemoteRef) rref = futureWatcher->result();
            if (rref) {
                m_eolReason = QString::fromUtf8(flatpak_remote_ref_get_eol(rref));
                Q_EMIT eolReasonChanged();
            }
            futureWatcher->deleteLater();
        });

        futureWatcher->setFuture(QtConcurrent::run(qobject_cast<FlatpakBackend *>(backend())->threadPool(),
                                                   &FlatpakRunnables::findRemoteRef,
                                                   this,
                                                   qobject_cast<FlatpakBackend *>(backend())->cancellable()));
        return {};
    }
    return m_eolReason.value_or(QString());
}

void FlatpakResource::loadPermissions()
{
    QByteArray metaDataBytes = FlatpakRunnables::fetchMetadata(this, NULL);

    QTemporaryFile f;
    if (!f.open()) {
        return;
    }
    f.write(metaDataBytes);
    f.close();

    KDesktopFile parser(f.fileName());

    QString brief, description;

    bool fullSessionBusAccess = false;
    bool fullSystemBusAccess = false;

    const KConfigGroup contextGroup = parser.group("Context");
    const QString shared = contextGroup.readEntry("shared", QString());
    if (shared.contains("network")) {
        brief = i18n("Network Access");
        description = i18n("Can access the internet");
        m_permissions.append(FlatpakPermission(brief, description, "network-wireless"));
    }

    const QString sockets = contextGroup.readEntry("sockets", QString());
    if (sockets.contains("session-bus")) {
        brief = i18n("Session Bus Access");
        description = i18n("Access is granted to the entire Session Bus");
        m_permissions.append(FlatpakPermission(brief, description, "system-save-session"));
        fullSessionBusAccess = true;
    }
    if (sockets.contains("system-bus")) {
        brief = i18n("System Bus Access");
        description = i18n("Access is granted to the entire System Bus");
        m_permissions.append(FlatpakPermission(brief, description, "system-save-session"));
        fullSystemBusAccess = true;
    }
    if (sockets.contains("ssh-auth")) {
        brief = i18n("Remote Login Access");
        description = i18n("Can initiate remote login requests using the SSH protocol");
        m_permissions.append(FlatpakPermission(brief, description, "x-shape-connection"));
    }
    if (sockets.contains("pcsc")) {
        brief = i18n("Smart Card Access");
        description = i18n("Can integrate and communicate with smart cards");
        m_permissions.append(FlatpakPermission(brief, description, "network-card"));
    }
    if (sockets.contains("cups")) {
        brief = i18n("Printer Access");
        description = i18n("Can integrate and communicate with printers");
        m_permissions.append(FlatpakPermission(brief, description, "document-print"));
    }
    if (sockets.contains("gpg-agent")) {
        brief = i18n("GPG Agent");
        description = i18n("Allows access to the GPG cryptography service, generally used for signing and reading signed documents");
        m_permissions.append(FlatpakPermission(brief, description, "gpg"));
    }

    const QString features = contextGroup.readEntry("features", QString());
    if (features.contains("bluetooth")) {
        brief = i18n("Bluetooth Access");
        description = i18n("Can integrate and communicate with Bluetooth devices");
        m_permissions.append(FlatpakPermission(brief, description, "network-bluetooth"));
    }
    if (features.contains("devel")) {
        brief = i18n("Low-Level System Access");
        description = i18n("Can make low-level system calls (e.g. ptrace)");
        m_permissions.append(FlatpakPermission(brief, description, "project-development"));
    }

    const QString devices = contextGroup.readEntry("devices", QString());
    if (devices.contains("all")) {
        brief = i18n("Device Access");
        description = i18n("Can communicate with and control built-in or connected hardware devices");
        m_permissions.append(FlatpakPermission(brief, description, "preferences-devices-tree"));
    }
    if (devices.contains("kvm")) {
        brief = i18n("Kernel-based Virtual Machine Access");
        description = i18n("Allows running other operating systems as guests in virtual machines");
        m_permissions.append(FlatpakPermission(brief, description, "virtualbox"));
    }

    const QString filesystems = contextGroup.readEntry("filesystems", QString());
    const auto dirs = QStringView(filesystems).split(';', Qt::SkipEmptyParts);
    QStringList homeList, systemList;
    bool home_ro = false, home_rw = false, home_cr = false, homeAccess = false;
    bool system_ro = false, system_rw = false, system_cr = false, systemAccess = false;

    bool hasHostRW = false;

    for (const QStringView &dir : dirs) {
        if (dir == QLatin1String("xdg-config/kdeglobals:ro")) {
            // Ignore notifying about the global file being visible, since it's intended by design
            continue;
        }

        int separator = dir.lastIndexOf(':');
        const QStringView postfix = separator > 0 ? dir.mid(separator) : QStringView();
        const QStringView symbolicName = dir.left(separator);
        const QString id = translateSymbolicName(symbolicName);
        if ((dir.contains(QLatin1String("home")) || dir.contains(QChar('~')))) {
            if (postfix == QLatin1String(":ro")) {
                homeList << i18n("%1 (read-only)", id);
                home_ro = true;
            } else if (postfix == QLatin1String(":create")) {
                homeList << i18n("%1 (can create files)", id);
                home_cr = true;
            } else {
                homeList << i18n("%1 (read & write) ", id);
                home_rw = true;
            }
            homeAccess = true;
        } else if (!hasHostRW) {
            if (postfix == QLatin1String(":ro")) {
                systemList << i18n("%1 (read-only)", id);
                system_ro = true;
            } else if (postfix == QLatin1String(":create")) {
                systemList << i18n("%1 (can create files)", id);
                system_cr = true;
            } else {
                // Once we have host in rw, no need to list the rest
                if (symbolicName == QLatin1String("host")) {
                    hasHostRW = true;
                    systemList.clear();
                }

                systemList << i18n("%1 (read & write) ", id);
                system_rw = true;
            }
            systemAccess = true;
        }
    }

    QString appendText = "\n- " + homeList.join("\n- ");
    if (homeAccess) {
        brief = i18n("Home Folder Access");
        if (home_rw && home_ro && home_cr) {
            description =
                i18n("Can read, write, and create files in the following locations in your home folder without asking permission first: %1", appendText);
        } else if (home_rw && !home_cr) {
            description = i18n("Can read and write files in the following locations in your home folder without asking permission first: %1", appendText);
        } else if (home_ro && !home_cr && !home_rw) {
            description = i18n("Can read files in the following locations in your home folder without asking permission first: %1", appendText);
        } else {
            description = i18n("Can access files in the following locations in your home folder without asking permission first: %1", appendText);
        }
        m_permissions.append(FlatpakPermission(brief, description, "inode-directory"));
    }
    appendText = "\n- " + systemList.join("\n- ");
    if (systemAccess) {
        brief = i18n("System Folder Access");
        if (system_rw && system_ro && system_cr) {
            description = i18n("Can read, write, and create system files in the following locations without asking permission first: %1", appendText);
        } else if (system_rw && !system_cr) {
            description = i18n("Can read and write system files in the following locations without asking permission first: %1", appendText);
        } else if (system_ro && !system_cr && !system_rw) {
            description = i18n("Can read system files in the following locations without asking permission first: %1", appendText);
        } else {
            description = i18n("Can access system files in the following locations without asking permission first: %1", appendText);
        }
        m_permissions.append(FlatpakPermission(brief, description, "inode-directory"));
    }

    if (!fullSessionBusAccess) {
        const KConfigGroup sessionBusGroup = parser.group("Session Bus Policy");
        if (sessionBusGroup.exists()) {
            const QStringList busList = sessionBusGroup.keyList();
            brief = i18n("Session Bus Access");
            description = i18n("Can communicate with other applications and processes in the same desktop session using the following communication protocols: %1",
                           "\n- " + busList.join("\n- "));
            m_permissions.append(FlatpakPermission(brief, description, "system-save-session"));
        }
    }

    if (!fullSystemBusAccess) {
        const KConfigGroup systemBusGroup = parser.group("System Bus Policy");
        if (systemBusGroup.exists()) {
            const QStringList busList = systemBusGroup.keyList();
            brief = i18n("System Bus Access");
            description =
                i18n("Can communicate with all applications and system services using the following communication protocols: %1", "\n- " + busList.join("\n- "));
            m_permissions.append(FlatpakPermission(brief, description, "system-save-session"));
        }
    }
}

QString FlatpakResource::dataLocation() const
{
    auto id = m_appdata.bundle(AppStream::Bundle::KindFlatpak).id().section('/', 0, 1);
    if (id.isEmpty()) {
        return {};
    }
    return QDir::homePath() + QLatin1String("/.var/") + id;
}

bool FlatpakResource::hasData() const
{
    return !dataLocation().isEmpty() && QDir(dataLocation()).exists();
}

void FlatpakResource::clearUserData()
{
    const auto location = dataLocation();
    if (location.isEmpty()) {
        qWarning() << "Failed find location for" << name();
        return;
    }

    if (!QDir(location).removeRecursively()) {
        qWarning() << "Failed to remove location" << location;
    }
    Q_EMIT hasDataChanged();
}

int FlatpakResource::versionCompare(FlatpakResource *resource) const
{
    const QString other = resource->availableVersion();
    return AppStream::Utils::vercmpSimple(availableVersion(), other);
}

QString FlatpakResource::contentRatingDescription() const
{
    return AppStreamUtils::contentRatingDescription(m_appdata);
}

QString FlatpakResource::contentRatingText() const
{
    return AppStreamUtils::contentRatingText(m_appdata);
}

AbstractResource::ContentIntensity FlatpakResource::contentRatingIntensity() const
{
    return AppStreamUtils::contentRatingIntensity(m_appdata);
}

uint FlatpakResource::contentRatingMinimumAge() const
{
    return AppStreamUtils::contentRatingMinimumAge(m_appdata);
}
