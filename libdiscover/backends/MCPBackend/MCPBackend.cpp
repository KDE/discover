/*
 *   SPDX-FileCopyrightText: 2025 Yakup Atahanov
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "MCPBackend.h"
#include "MCPResource.h"
#include "MCPTransaction.h"
#include "libdiscover_backend_mcp_debug.h"

#include <resources/StandardBackendUpdater.h>
#include <Transaction/Transaction.h>
#include <Category/Category.h>

#include <KLocalizedString>
#include <KPluginFactory>

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QProcess>
#include <QStandardPaths>
#include <QTextStream>
#include <QTimer>

DISCOVER_BACKEND_PLUGIN(MCPBackend)

using namespace Qt::StringLiterals;

MCPBackend::MCPBackend(QObject *parent)
    : AbstractResourcesBackend(parent)
    , m_updater(new StandardBackendUpdater(this))
    , m_networkManager(new QNetworkAccessManager(this))
{
    qCDebug(LIBDISCOVER_BACKEND_MCP_LOG) << "MCPBackend: Initializing MCP backend";

    connect(m_networkManager, &QNetworkAccessManager::finished,
            this, &MCPBackend::onRegistryFetched);
    connect(m_updater, &StandardBackendUpdater::updatesCountChanged,
            this, &MCPBackend::updatesCountChanged);

    // Load registry sources from config
    loadSourcesConfig();

    // Load installed servers from installed/{id}/manifest.json files (primary source of truth)
    loadInstalledServers();

    // Also load from mcp.json as fallback for older installations
    loadMcpJson();

    // Load any cached registry data
    loadCachedRegistries();

    // Fetch fresh registry data in the background (if sources configured)
    if (!m_registrySources.isEmpty()) {
        QTimer::singleShot(1000, this, &MCPBackend::fetchOnlineRegistries);
    }

    qCDebug(LIBDISCOVER_BACKEND_MCP_LOG) << "MCPBackend: Initialization complete,"
        << m_resources.count() << "resources loaded,"
        << m_registrySources.count() << "registry sources configured";
}

MCPBackend::~MCPBackend()
{
    qDeleteAll(m_resources);
}

QString MCPBackend::displayName() const
{
    return i18n("MCP Servers");
}

int MCPBackend::updatesCount() const
{
    return m_updater->updatesCount();
}

int MCPBackend::fetchingUpdatesProgress() const
{
    return m_fetchProgress;
}

AbstractBackendUpdater *MCPBackend::backendUpdater() const
{
    return m_updater;
}

AbstractReviewsBackend *MCPBackend::reviewsBackend() const
{
    return nullptr;
}

ResultsStream *MCPBackend::search(const AbstractResourcesBackend::Filters &filter)
{
    QVector<StreamResult> results;

    // Handle URL-based search (mcp://server-id)
    if (!filter.resourceUrl.isEmpty()) {
        if (filter.resourceUrl.scheme() == u"mcp"_s) {
            const QString id = filter.resourceUrl.host();
            if (MCPResource *res = m_resources.value(id)) {
                results.append(StreamResult(res));
            }
        }
        return new ResultsStream(u"MCPStream"_s, results);
    }

    // Filter resources
    for (MCPResource *resource : std::as_const(m_resources)) {
        // State filter
        if (filter.state != AbstractResource::Broken) {
            if (resource->state() < filter.state) {
                continue;
            }
        }

        // Category filter
        if (filter.category) {
            bool categoryMatch = false;
            const auto categoryName = filter.category->name().toLower();

            if (resource->hasCategory(categoryName)) {
                categoryMatch = true;
            }

            if (categoryName == u"mcp servers"_s || categoryName == u"mcp"_s) {
                categoryMatch = true;
            }

            if (!categoryMatch) {
                continue;
            }
        }

        // Search text filter
        if (!filter.search.isEmpty()) {
            const QString search = filter.search.toLower();
            if (!resource->name().toLower().contains(search) &&
                !resource->comment().toLower().contains(search) &&
                !resource->packageName().toLower().contains(search)) {
                continue;
            }
        }

        results.append(StreamResult(resource));
    }

    return new ResultsStream(u"MCPStream"_s, results);
}

Transaction *MCPBackend::installApplication(AbstractResource *app, const AddonList &addons)
{
    Q_UNUSED(addons)
    return installApplication(app);
}

Transaction *MCPBackend::installApplication(AbstractResource *app)
{
    MCPResource *resource = qobject_cast<MCPResource *>(app);
    if (!resource) {
        return nullptr;
    }

    return new MCPTransaction(resource, Transaction::InstallRole);
}

Transaction *MCPBackend::removeApplication(AbstractResource *app)
{
    MCPResource *resource = qobject_cast<MCPResource *>(app);
    if (!resource) {
        return nullptr;
    }

    return new MCPTransaction(resource, Transaction::RemoveRole);
}

void MCPBackend::checkForUpdates()
{
    if (m_fetching || m_registrySources.isEmpty()) {
        return;
    }

    m_fetching = true;
    m_fetchProgress = 0;
    m_currentFetchIndex = 0;
    Q_EMIT fetchingUpdatesProgressChanged();

    fetchOnlineRegistries();
}

InlineMessage *MCPBackend::explainDysfunction() const
{
    // Don't report dysfunction - just silently have no content if no sources
    // This prevents blocking other backends
    return nullptr;
}

void MCPBackend::addRegistrySource(const QString &url)
{
    if (!m_registrySources.contains(url)) {
        m_registrySources.append(url);
        saveSourcesConfig();
        Q_EMIT registrySourcesChanged();

        // Fetch from the new source
        fetchOnlineRegistries();
    }
}

void MCPBackend::removeRegistrySource(const QString &url)
{
    if (m_registrySources.removeOne(url)) {
        saveSourcesConfig();
        Q_EMIT registrySourcesChanged();
    }
}

MCPResource *MCPBackend::resourceById(const QString &id) const
{
    return m_resources.value(id);
}

void MCPBackend::bootstrapUserConfig()
{
    const QString userConfigDir = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation);
    const QString mcpConfigDir = userConfigDir + u"/mcp"_s;
    const QString sourcesPath = mcpConfigDir + u"/sources.list"_s;
    const QString configPath = mcpConfigDir + u"/config.json"_s;

    // Create ~/.config/mcp/ if it doesn't exist
    QDir().mkpath(mcpConfigDir);

    // Create sources.list with default registry if it doesn't exist
    if (!QFile::exists(sourcesPath)) {
        QFile file(sourcesPath);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream stream(&file);
            stream << "# MCP Registry Sources\n";
            stream << "# Each line is a URL to a registry JSON file.\n";
            stream << "# Add your own registries below, or remove the default.\n\n";
            stream << "# Default MCP server registry\n";
            stream << defaultRegistryUrl() << "\n";
            file.close();
            qCDebug(LIBDISCOVER_BACKEND_MCP_LOG) << "MCPBackend: Created default sources.list at" << sourcesPath;
        }
    }

    // Create config.json if it doesn't exist (ready for parameter storage)
    if (!QFile::exists(configPath)) {
        QFile file(configPath);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            file.write("{\n  \"servers\": {}\n}\n");
            file.close();
            qCDebug(LIBDISCOVER_BACKEND_MCP_LOG) << "MCPBackend: Created default config.json at" << configPath;
        }
    }
}

void MCPBackend::loadSourcesConfig()
{
    m_registrySources.clear();

    // Ensure user config directory and defaults exist on first run
    bootstrapUserConfig();

    // Load from user config first
    const QString userConfigDir = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation);
    const QString userSourcesPath = userConfigDir + u"/mcp/sources.list"_s;

    // Then system config
    const QStringList configDirs = {
        userSourcesPath,
        u"/etc/mcp/sources.list"_s
    };

    for (const QString &configPath : configDirs) {
        QFile file(configPath);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream stream(&file);
            while (!stream.atEnd()) {
                QString line = stream.readLine().trimmed();
                // Skip empty lines and comments
                if (line.isEmpty() || line.startsWith(u'#')) {
                    continue;
                }
                if (!m_registrySources.contains(line)) {
                    m_registrySources.append(line);
                }
            }
            file.close();
        }
    }

    // Always ensure the default registry is present
    const QString defaultUrl = defaultRegistryUrl();
    if (!defaultUrl.isEmpty() && !m_registrySources.contains(defaultUrl)) {
        m_registrySources.prepend(defaultUrl);
    }

    qCDebug(LIBDISCOVER_BACKEND_MCP_LOG) << "MCPBackend: Loaded" << m_registrySources.count() << "registry sources";
}

void MCPBackend::saveSourcesConfig()
{
    const QString userConfigDir = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation);
    const QString mcpConfigDir = userConfigDir + u"/mcp"_s;
    const QString sourcesPath = mcpConfigDir + u"/sources.list"_s;

    QDir().mkpath(mcpConfigDir);

    QFile file(sourcesPath);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream stream(&file);
        stream << "# MCP Registry Sources\n";
        stream << "# Each line is a URL to a registry JSON file\n\n";
        for (const QString &source : m_registrySources) {
            stream << source << "\n";
        }
        file.close();
    }
}

QString MCPBackend::systemMcpDir()
{
    return u"/usr/share/mcp"_s;
}

QString MCPBackend::systemMcpJsonPath()
{
    return systemMcpDir() + u"/mcp.json"_s;
}

QString MCPBackend::systemInstalledDir()
{
    return systemMcpDir() + u"/installed"_s;
}

QString MCPBackend::serverInstallDir(const QString &serverId)
{
    return systemInstalledDir() + u"/"_s + serverId;
}

QString MCPBackend::serverManifestPath(const QString &serverId)
{
    return serverInstallDir(serverId) + u"/manifest.json"_s;
}

QString MCPBackend::defaultRegistryUrl()
{
    return u"https://raw.githubusercontent.com/YakupAtahanov/mcp-registry/refs/heads/main/registry.json"_s;
}

bool MCPBackend::writeServerManifest(const QString &serverId, const QJsonObject &manifest)
{
    const QString manifestPath = serverManifestPath(serverId);
    const QJsonDocument doc(manifest);
    return privilegedWriteFile(manifestPath, doc.toJson(QJsonDocument::Indented));
}

void MCPBackend::loadInstalledServers()
{
    const QDir installedDir(systemInstalledDir());
    if (!installedDir.exists()) {
        qCDebug(LIBDISCOVER_BACKEND_MCP_LOG) << "MCPBackend: No installed directory at" << systemInstalledDir();
        return;
    }

    const QStringList serverDirs = installedDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    int count = 0;
    for (const QString &dirName : serverDirs) {
        const QString manifestPath = serverManifestPath(dirName);
        QFile file(manifestPath);
        if (!file.open(QIODevice::ReadOnly)) {
            qCDebug(LIBDISCOVER_BACKEND_MCP_LOG) << "MCPBackend: No manifest.json in" << dirName;
            continue;
        }

        const QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
        file.close();

        if (!doc.isObject()) {
            qCWarning(LIBDISCOVER_BACKEND_MCP_LOG) << "MCPBackend: Invalid manifest.json in" << dirName;
            continue;
        }

        const QJsonObject serverObj = doc.object();
        MCPResource *resource = createResourceFromJson(serverObj);
        if (resource) {
            resource->setState(AbstractResource::Installed);
            const QString installedVersion = serverObj[u"installedVersion"_s].toString();
            resource->setInstalledVersion(installedVersion);
            resource->loadUserConfiguration();
            addResource(resource);
            ++count;
        }
    }

    qCDebug(LIBDISCOVER_BACKEND_MCP_LOG) << "MCPBackend: Loaded" << count
        << "installed servers from" << systemInstalledDir();
}

void MCPBackend::loadMcpJson()
{
    const QString mcpJsonPath = systemMcpJsonPath();
    QFile file(mcpJsonPath);
    if (!file.open(QIODevice::ReadOnly)) {
        qCDebug(LIBDISCOVER_BACKEND_MCP_LOG) << "MCPBackend: No mcp.json found at" << mcpJsonPath;
        return;
    }

    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();

    if (!doc.isObject()) {
        qCWarning(LIBDISCOVER_BACKEND_MCP_LOG) << "MCPBackend: Invalid mcp.json format";
        return;
    }

    const QJsonObject root = doc.object();
    const QJsonArray servers = root[u"servers"_s].toArray();

    for (const QJsonValue &serverValue : servers) {
        if (!serverValue.isObject()) {
            continue;
        }

        const QJsonObject serverObj = serverValue.toObject();
        MCPResource *resource = createResourceFromJson(serverObj);
        if (resource) {
            resource->setState(AbstractResource::Installed);
            const QString installedVersion = serverObj[u"installedVersion"_s].toString();
            resource->setInstalledVersion(installedVersion);

            // Load user-specific configuration (API keys, etc.) from ~/.config/mcp/config.json
            resource->loadUserConfiguration();

            addResource(resource);
        }
    }

    qCDebug(LIBDISCOVER_BACKEND_MCP_LOG) << "MCPBackend: Loaded" << m_resources.count()
        << "installed servers from" << mcpJsonPath;
}

bool MCPBackend::privilegedWriteFile(const QString &filePath, const QByteArray &content)
{
    // Write content to a temporary file first, then use pkexec to move it into place
    const QString tempPath = QStandardPaths::writableLocation(QStandardPaths::TempLocation)
        + u"/mcp-write-"_s + QString::number(QDateTime::currentMSecsSinceEpoch()) + u".tmp"_s;

    // Write to temp file (user-writable location)
    QFile tempFile(tempPath);
    if (!tempFile.open(QIODevice::WriteOnly)) {
        qCWarning(LIBDISCOVER_BACKEND_MCP_LOG) << "MCPBackend: Failed to write temp file:" << tempPath;
        return false;
    }
    tempFile.write(content);
    tempFile.close();

    // Ensure parent directory exists via pkexec
    const QString parentDir = QFileInfo(filePath).absolutePath();
    QProcess mkdirProcess;
    mkdirProcess.start(u"pkexec"_s, {u"mkdir"_s, u"-p"_s, parentDir});
    mkdirProcess.waitForFinished(30000);

    // Use pkexec to copy temp file to the target (atomic: cp then set permissions)
    QProcess cpProcess;
    cpProcess.start(u"pkexec"_s, {u"cp"_s, u"--no-preserve=mode"_s, tempPath, filePath});
    cpProcess.waitForFinished(30000);

    // Clean up temp file
    QFile::remove(tempPath);

    if (cpProcess.exitCode() != 0) {
        qCWarning(LIBDISCOVER_BACKEND_MCP_LOG) << "MCPBackend: pkexec cp failed:"
            << cpProcess.readAllStandardError();
        return false;
    }

    return true;
}

bool MCPBackend::addServerToMcpJson(const QJsonObject &serverData)
{
    // Read existing mcp.json
    QJsonObject root;
    QJsonArray servers;
    {
        QFile file(systemMcpJsonPath());
        if (file.open(QIODevice::ReadOnly)) {
            const QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
            file.close();
            if (doc.isObject()) {
                root = doc.object();
                servers = root[u"servers"_s].toArray();
            }
        }
    }

    // Remove existing entry with same ID if present (for upgrades)
    const QString serverId = serverData[u"id"_s].toString();
    for (int i = 0; i < servers.count(); ++i) {
        if (servers[i].toObject()[u"id"_s].toString() == serverId) {
            servers.removeAt(i);
            break;
        }
    }

    // Add the new server entry
    servers.append(serverData);
    root[u"servers"_s] = servers;

    const QJsonDocument doc(root);
    return privilegedWriteFile(systemMcpJsonPath(), doc.toJson(QJsonDocument::Indented));
}

bool MCPBackend::removeServerFromMcpJson(const QString &serverId)
{
    // Read existing mcp.json
    QJsonObject root;
    QJsonArray servers;
    {
        QFile file(systemMcpJsonPath());
        if (file.open(QIODevice::ReadOnly)) {
            const QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
            file.close();
            if (doc.isObject()) {
                root = doc.object();
                servers = root[u"servers"_s].toArray();
            }
        }
    }

    // Find and remove the server entry
    bool found = false;
    for (int i = 0; i < servers.count(); ++i) {
        if (servers[i].toObject()[u"id"_s].toString() == serverId) {
            servers.removeAt(i);
            found = true;
            break;
        }
    }

    if (!found) {
        qCWarning(LIBDISCOVER_BACKEND_MCP_LOG) << "MCPBackend: Server not found in mcp.json:" << serverId;
        return false;
    }

    root[u"servers"_s] = servers;

    const QJsonDocument doc(root);
    return privilegedWriteFile(systemMcpJsonPath(), doc.toJson(QJsonDocument::Indented));
}

void MCPBackend::loadCachedRegistries()
{
    const QString cacheDir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
    const QDir dir(cacheDir + u"/mcp-registries"_s);

    if (!dir.exists()) {
        return;
    }

    const QStringList cacheFiles = dir.entryList({u"*.json"_s}, QDir::Files);
    for (const QString &cacheFile : cacheFiles) {
        QFile file(dir.absoluteFilePath(cacheFile));
        if (file.open(QIODevice::ReadOnly)) {
            const QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
            file.close();

            if (doc.isObject()) {
                const QJsonArray servers = doc.object()[u"servers"_s].toArray();
                parseRegistryData(servers);
            }
        }
    }
}

void MCPBackend::fetchOnlineRegistries()
{
    if (m_registrySources.isEmpty()) {
        m_fetching = false;
        m_fetchProgress = 100;
        Q_EMIT fetchingUpdatesProgressChanged();
        return;
    }

    m_currentFetchIndex = 0;
    m_fetching = true;
    fetchNextRegistry();
}

void MCPBackend::fetchNextRegistry()
{
    if (m_currentFetchIndex >= m_registrySources.count()) {
        // Done fetching all registries
        m_fetching = false;
        m_fetchProgress = 100;
        Q_EMIT fetchingUpdatesProgressChanged();
        Q_EMIT contentsChanged();
        return;
    }

    const QString &sourceUrl = m_registrySources.at(m_currentFetchIndex);
    qCDebug(LIBDISCOVER_BACKEND_MCP_LOG) << "MCPBackend: Fetching registry:" << sourceUrl;

    const QUrl url(sourceUrl);
    QNetworkRequest request(url);
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                        QNetworkRequest::NoLessSafeRedirectPolicy);
    request.setHeader(QNetworkRequest::UserAgentHeader,
                     u"KDE Discover MCP Backend/1.0"_s);

    m_networkManager->get(request);

    // Update progress
    m_fetchProgress = (m_currentFetchIndex * 100) / m_registrySources.count();
    Q_EMIT fetchingUpdatesProgressChanged();
}

void MCPBackend::onRegistryFetched(QNetworkReply *reply)
{
    reply->deleteLater();

    const QString sourceUrl = reply->url().toString();

    if (reply->error() == QNetworkReply::NoError) {
        const QByteArray data = reply->readAll();
        const QJsonDocument doc = QJsonDocument::fromJson(data);

        if (doc.isObject()) {
            // Cache the registry data
            const QString cacheDir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
            const QString registryCacheDir = cacheDir + u"/mcp-registries"_s;
            QDir().mkpath(registryCacheDir);

            // Use URL hash as filename
            const QString cacheFileName = QString::number(qHash(sourceUrl)) + u".json"_s;
            const QString cachePath = registryCacheDir + u"/"_s + cacheFileName;

            QFile cacheFile(cachePath);
            if (cacheFile.open(QIODevice::WriteOnly)) {
                cacheFile.write(data);
                cacheFile.close();
            }

            // Parse the registry
            const QJsonArray servers = doc.object()[u"servers"_s].toArray();
            parseRegistryData(servers);

            qCDebug(LIBDISCOVER_BACKEND_MCP_LOG) << "MCPBackend: Loaded" << servers.count() << "servers from" << sourceUrl;
        }
    } else {
        qCWarning(LIBDISCOVER_BACKEND_MCP_LOG) << "MCPBackend: Failed to fetch registry" << sourceUrl << ":" << reply->errorString();
    }

    // Fetch next registry
    m_currentFetchIndex++;
    fetchNextRegistry();
}

void MCPBackend::parseRegistryData(const QJsonArray &servers)
{
    for (const QJsonValue &serverValue : servers) {
        if (!serverValue.isObject()) {
            continue;
        }

        const QJsonObject serverObj = serverValue.toObject();
        const QString id = serverObj[u"id"_s].toString();

        MCPResource *existing = m_resources.value(id);
        if (existing) {
            // Check for updates
            const QString availableVersion = serverObj[u"version"_s].toString();
            if (!existing->installedVersion().isEmpty() &&
                existing->installedVersion() != availableVersion) {
                existing->setState(AbstractResource::Upgradeable);
            }
            continue;
        }

        MCPResource *resource = createResourceFromJson(serverObj);
        if (resource) {
            addResource(resource);
        }
    }
}

void MCPBackend::addResource(MCPResource *resource)
{
    const QString id = resource->packageName();
    if (m_resources.contains(id)) {
        delete resource;
        return;
    }

    m_resources.insert(id, resource);
    connect(resource, &MCPResource::stateChanged,
            this, &MCPBackend::updatesCountChanged);
}

MCPResource *MCPBackend::createResourceFromJson(const QJsonObject &data)
{
    const QString id = data[u"id"_s].toString();
    if (id.isEmpty()) {
        return nullptr;
    }

    return new MCPResource(data, this);
}

#include "MCPBackend.moc"
#include "moc_MCPBackend.cpp"
