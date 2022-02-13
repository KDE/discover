/*
 *   SPDX-FileCopyrightText: 2013 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *   SPDX-FileCopyrightText: 2017 Jan Grulich <jgrulich@redhat.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "FlatpakResource.h"
#include "FlatpakBackend.h"
#include "FlatpakFetchDataJob.h"
#include "FlatpakPermission.h"
#include "FlatpakSourcesBackend.h"
#include "config-paths.h"

#include <Transaction/AddonList.h>

#include <AppStreamQt/icon.h>
#include <AppStreamQt/screenshot.h>
#include <appstream/AppStreamUtils.h>

#include <KFormat>
#include <KLocalizedString>

#include <AppStreamQt/release.h>
#include <QDebug>
#include <QDesktopServices>
#include <QDir>
#include <QFileInfo>
#include <QIcon>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QProcess>
#include <QStringList>
#include <QTimer>
#include <QUrlQuery>

static QString iconCachePath(const AppStream::Icon &icon)
{
    Q_ASSERT(icon.kind() == AppStream::Icon::KindRemote);
    return QStringLiteral("%1/icons/%2").arg(QStandardPaths::writableLocation(QStandardPaths::CacheLocation), icon.url().fileName());
}

const QStringList FlatpakResource::m_objects({QStringLiteral("qrc:/qml/FlatpakAttention.qml")});
const QStringList FlatpakResource::m_bottomObjects({QStringLiteral("qrc:/qml/PermissionsList.qml")});

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
    if (!icons.isEmpty()) {
        for (const AppStream::Icon &icon : icons) {
            if (icon.kind() == AppStream::Icon::KindRemote) {
                const QString fileName = iconCachePath(icon);
                if (!QFileInfo::exists(fileName)) {
                    const QDir cacheDir(QStandardPaths::writableLocation(QStandardPaths::CacheLocation));
                    // Create $HOME/.cache/discover/icons folder
                    cacheDir.mkdir(QStringLiteral("icons"));
                    QNetworkAccessManager *manager = new QNetworkAccessManager(this);
                    connect(manager, &QNetworkAccessManager::finished, this, [this, icon, fileName, manager](QNetworkReply *reply) {
                        if (reply->error() == QNetworkReply::NoError) {
                            QByteArray iconData = reply->readAll();
                            QFile file(fileName);
                            if (file.open(QIODevice::WriteOnly)) {
                                file.write(iconData);
                            }
                            file.close();
                            Q_EMIT iconChanged();
                        }
                        manager->deleteLater();
                    });
                    manager->get(QNetworkRequest(icon.url()));
                }
            }
        }
    }
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
    QString theBranch = branch();
    if (theBranch.isEmpty()) {
        theBranch = i18n("Unknown");
    }

    if (!m_availableVersion.isEmpty()) {
        return i18nc("version (branch)", "%1 (%2)", m_availableVersion, theBranch);
    }

    return theBranch;
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
            case AppStream::Icon::KindCached:
                ret.addFile(icon.url().toLocalFile(), icon.size());
                break;
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
    QString version = branch();
    if (version.isEmpty()) {
        version = i18n("Unknown");
    }

    g_autoptr(FlatpakInstalledRef) ref = qobject_cast<FlatpakBackend *>(backend())->getInstalledRefForApp(this);
    if (ref) {
        const char *appdataVersion = flatpak_installed_ref_get_appdata_version(ref);
        if (appdataVersion) {
            return i18nc("version (branch)", "%1 (%2)", appdataVersion, version);
        }
    }
    return version;
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
    QString description = m_appdata.description();

    return description;
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

Q_INVOKABLE QAbstractListModel *FlatpakResource::showPermissions()
{
    if (m_permissions.empty()) {
        loadPermissions();
    }
    return new FlatpakPermissionsModel(m_permissions);
}

Q_INVOKABLE int FlatpakResource::permissionCount()
{
    if (m_permissions.empty()) {
        loadPermissions();
    }
    return m_permissions.count();
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
    const QString desktopFile = installPath() + QLatin1String("/export/share/applications/") + appstreamId();
    const QString runservice = QStringLiteral(CMAKE_INSTALL_FULL_LIBEXECDIR_KF5 "/discover/runservice");
    if (QFile::exists(desktopFile) && QFile::exists(runservice)) {
        QProcess::startDetached(runservice, {desktopFile});
        return;
    }

    g_autoptr(GCancellable) cancellable = g_cancellable_new();
    g_autoptr(GError) localError = nullptr;

    if (!flatpak_installation_launch(m_installation,
                                     flatpakName().toUtf8().constData(),
                                     arch().toUtf8().constData(),
                                     branch().toUtf8().constData(),
                                     nullptr,
                                     cancellable,
                                     &localError)) {
        qWarning() << "Failed to launch " << m_appdata.name() << ": " << localError->message;
    }
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
    return m_resourceFile.isEmpty() ? QUrl(QStringLiteral("appstream://") + appstreamId()) : m_resourceFile;
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

void FlatpakResource::loadPermissions()
{
    QByteArray metaDataBytes = FlatpakRunnables::fetchMetadata(this, NULL);
    QString metaData(metaDataBytes);

    QString name, category, brief, description, value = "on";

    category = "shared";
    if (metaData.contains("network")) {
        name = i18n("network");
        brief = i18n("Network Access");
        description = i18n("Can access the internet");
        m_permissions.push_back(FlatpakPermission(name, category, value, brief, description, "network-wireless"));
    }

    category = "socket";
    if (metaData.contains("session-bus")) {
        name = i18n("session-bus");
        brief = i18n("Session Bus Access");
        description = i18n("Access is granted to the entire Session Bus");
        m_permissions.push_back(FlatpakPermission(name, category, value, brief, description, "system-save-session"));
    }
    if (metaData.contains("system-bus")) {
        name = i18n("system-bus");
        brief = i18n("System Bus Access");
        description = i18n("Access is granted to the entire System Bus");
        m_permissions.push_back(FlatpakPermission(name, category, value, brief, description, "system-save-session"));
    }
    if (metaData.contains("ssh-auth")) {
        name = i18n("ssh-auth");
        brief = i18n("Remote Login Access");
        description = i18n("Can initiate remote login requests using the SSH protocol");
        m_permissions.push_back(FlatpakPermission(name, category, value, brief, description, "x-shape-connection"));
    }
    if (metaData.contains("pcsc")) {
        name = i18n("pspc");
        brief = i18n("Smart Card Access");
        description = i18n("Can integrate and communicate with smart cards");
        m_permissions.push_back(FlatpakPermission(name, category, value, brief, description, "network-card"));
    }

    category = "devices";
    if (metaData.contains("kvm")) {
        name = i18n("kvm");
        brief = i18n("Kernel-based Virtual Machine Access");
        description = i18n("Allows running other operating systems as guests in virtual machines");
        m_permissions.push_back(FlatpakPermission(name, category, value, brief, description, "virtualbox"));
    }
    if (metaData.contains("all")) {
        name = i18n("all");
        brief = i18n("Device Access");
        description = i18n("Can communicate with and control built-in or connected hardware devices");
        m_permissions.push_back(FlatpakPermission(name, category, value, brief, description, "preferences-devices-tree"));
    }

    category = "filesystems";
    int fsStartIndex = metaData.indexOf(category);
    int fsPermStartIndex = metaData.indexOf('=', fsStartIndex) + 1;
    int fsPermEndIndex = metaData.indexOf('\n', fsPermStartIndex);
    QString dirs;
    for (int i = fsPermStartIndex; i < fsPermEndIndex; ++i) {
        dirs.push_back(metaData[i]);
    }
    QStringList homeList, systemList;
    bool home_ro = false, home_rw = false, home_cr = false, homeAccess = false;
    bool system_ro = false, system_rw = false, system_cr = false, systemAccess = false;
    for (int j = 0; j < dirs.count(';'); ++j) {
        QString dir = dirs.section(';', j, j);

        if ((dir.contains("home") || dir.contains("~"))) {
            if (dir.contains(":ro")) {
                homeList << dir.remove(":ro").append(i18n(" (read-only) "));
                home_ro = true;
            } else if (dir.contains(":create")) {
                homeList << dir.remove(":create").append(i18n(" (can create files) "));
                home_cr = true;
            } else {
                homeList << dir.remove(":rw").append(i18n(" (read & write) "));
                home_rw = true;
            }
            homeAccess = true;
        } else {
            if (dir.contains(":ro")) {
                systemList << dir.remove(":ro").append(i18n(" (read-only) "));
                system_ro = true;
            } else if (dir.contains(":create")) {
                systemList << dir.remove(":create").append(i18n(" (can create files) "));
                system_cr = true;
            } else {
                systemList << dir.remove(":rw").append(i18n(" (read & write) "));
                system_rw = true;
            }
            systemAccess = true;
        }
    }

    QString appendText = "\n- " + homeList.join("\n- ");
    if (homeAccess) {
        brief = i18n("Home Folder Access");
        QString accessLevel;
        if (home_rw && home_ro && home_cr) {
            description = i18n("Can read, write, and create files in the following locations in your home folder without asking permission first:").append(appendText);
        } else if (home_rw && !home_cr) {
            description = i18n("Can read and write files in the following locations in your home folder without asking permission first:").append(appendText);
        } else if (home_ro && !home_cr && !home_rw) {
            description = i18n("Can read files in the following locations in your home folder without asking permission first:").append(appendText);
        } else {
            description = i18n("Can access files in the following locations in your home folder without asking permission first:").append(appendText);
        }
        m_permissions.push_back(FlatpakPermission(i18n("filesystems"), category, value, brief, description, "inode-directory", homeList));
    }
    appendText = "\n- " + systemList.join("\n- ");
    if (systemAccess) {
        brief = i18n("System Folder Access");
        QString accessLevel;
        if (system_rw && system_ro && system_cr) {
            description = i18n("Can read, write, and create system files in the following locations without asking permission first:").append(appendText);
        } else if (system_rw && !system_cr) {
            description = i18n("Can read and write system files in the following locations without asking permission first:").append(appendText);
        } else if (system_ro && !system_cr && !system_rw) {
            description = i18n("Can read system files in the following locations without asking permission first:").append(appendText);
        } else {
            description = i18n("Can access system files in the following locations without asking permission first:").append(appendText);
        }
        m_permissions.push_back(FlatpakPermission(i18n("filesystems"), category, value, brief, description, "inode-directory", systemList));
    }

    category = "Session Bus Policy";
    int index = metaData.indexOf(category);
    if (index != -1) {
        index = metaData.indexOf('\n', index);
        QStringList busList;
        while (true) {
            QString policy;
            int j;
            for (j = index + 1; metaData[j] != '\n'; ++j) {
                policy.push_back(metaData[j]);
            }
            if (policy.isEmpty()) {
                break;
            }
            QStringList l = policy.split('=');
            busList << l[0];
            index = j;
        }
        name = i18n("Session Bus Policy");
        brief = i18n("Session Bus Access");
        description = i18n("Can communicate with other applications and processes in the same desktop session using the following communication protocols:").append("\n- " + busList.join("\n- "));
        m_permissions.push_back(FlatpakPermission(name, category, value, brief, description, "system-save-session", busList));
    }

    category = "System Bus Policy";
    index = metaData.indexOf(category);
    if (index != -1) {
        index = metaData.indexOf('\n', index);
        QStringList busList;
        while (true) {
            QString policy;
            int j;
            for (j = index + 1; metaData[j] != '\n'; ++j) {
                policy.push_back(metaData[j]);
            }
            if (policy.isEmpty()) {
                break;
            }
            QStringList l = policy.split('=');
            busList << l[0];
            index = j;
        }
        name = i18n("System Bus Policy");
        brief = i18n("System Bus Access");
        description = i18n("Can communicate with all applications and system services using the following communication protocols:").append("\n- " + busList.join("\n- "));
        m_permissions.push_back(FlatpakPermission(name, category, value, brief, description, "system-save-session", busList));
    }
}
