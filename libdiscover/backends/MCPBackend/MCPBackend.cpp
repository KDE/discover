/*
 *   SPDX-FileCopyrightText: 2025 Yakup Atahanov
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "MCPBackend.h"
#include "MCPResource.h"
#include "MCPTransaction.h"

#include <resources/StandardBackendUpdater.h>
#include <Transaction/Transaction.h>

#include <KLocalizedString>
#include <KPluginFactory>

#include <QDebug>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QStandardPaths>
#include <QTimer>

DISCOVER_BACKEND_PLUGIN(MCPBackend)

using namespace Qt::StringLiterals;

MCPBackend::MCPBackend(QObject *parent)
    : AbstractResourcesBackend(parent)
    , m_updater(new StandardBackendUpdater(this))
    , m_networkManager(new QNetworkAccessManager(this))
    , m_registryUrl(QString::fromLatin1(DEFAULT_REGISTRY_URL))
{
    connect(m_networkManager, &QNetworkAccessManager::finished,
            this, &MCPBackend::onRegistryFetched);
    connect(m_updater, &StandardBackendUpdater::updatesCountChanged,
            this, &MCPBackend::updatesCountChanged);

    // Load installed servers first
    loadInstalledServers();

    // Load any cached registry data
    loadRegistryCatalog();

    // Fetch fresh registry data in the background
    QTimer::singleShot(1000, this, &MCPBackend::fetchOnlineRegistry);
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
    // No reviews backend for now
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
            // Check if the resource matches any of the category's include filters
            bool categoryMatch = false;
            const auto categoryName = filter.category->name().toLower();

            // Check direct category match
            if (resource->hasCategory(categoryName)) {
                categoryMatch = true;
            }

            // Also check for "mcp" category
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
    if (m_fetching) {
        return;
    }

    m_fetching = true;
    m_fetchProgress = 0;
    Q_EMIT fetchingUpdatesProgressChanged();

    fetchOnlineRegistry();
}

InlineMessage *MCPBackend::explainDysfunction() const
{
    if (m_resources.isEmpty()) {
        return new InlineMessage(
            InlineMessage::Warning,
            u"dialog-warning"_s,
            i18n("No MCP servers available. Check your internet connection or registry URL.")
        );
    }
    return nullptr;
}

void MCPBackend::setRegistryUrl(const QString &url)
{
    if (m_registryUrl != url) {
        m_registryUrl = url;
        Q_EMIT registryUrlChanged();

        // Refetch with new URL
        fetchOnlineRegistry();
    }
}

MCPResource *MCPBackend::resourceById(const QString &id) const
{
    return m_resources.value(id);
}

void MCPBackend::loadInstalledServers()
{
    // Check user-specific installation directory
    const QString userDataDir = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation);
    const QString userMcpDir = userDataDir + u"/mcp/installed"_s;

    // Check system-wide installation directory
    const QStringList systemDirs = QStandardPaths::standardLocations(QStandardPaths::GenericDataLocation);

    QStringList searchDirs;
    searchDirs << userMcpDir;
    for (const QString &dir : systemDirs) {
        searchDirs << dir + u"/mcp/installed"_s;
    }

    for (const QString &dirPath : searchDirs) {
        const QDir dir(dirPath);
        if (!dir.exists()) {
            continue;
        }

        const QStringList manifests = dir.entryList({u"*.json"_s}, QDir::Files);
        for (const QString &manifest : manifests) {
            const QString filePath = dir.absoluteFilePath(manifest);
            QFile file(filePath);

            if (file.open(QIODevice::ReadOnly)) {
                const QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
                file.close();

                if (doc.isObject()) {
                    MCPResource *resource = createResourceFromJson(doc.object());
                    if (resource) {
                        resource->setState(AbstractResource::Installed);
                        const QString installedVersion = doc.object()[u"installedVersion"_s].toString();
                        resource->setInstalledVersion(installedVersion);
                        addResource(resource);
                    }
                }
            }
        }
    }

    qDebug() << "Loaded" << m_resources.count() << "installed MCP servers";
}

void MCPBackend::loadRegistryCatalog()
{
    // Load cached registry from disk
    const QString cacheDir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
    const QString cachePath = cacheDir + u"/mcp-registry.json"_s;

    QFile cacheFile(cachePath);
    if (cacheFile.open(QIODevice::ReadOnly)) {
        const QJsonDocument doc = QJsonDocument::fromJson(cacheFile.readAll());
        cacheFile.close();

        if (doc.isObject()) {
            const QJsonArray servers = doc.object()[u"servers"_s].toArray();
            parseRegistryData(servers);
        }
    }
}

void MCPBackend::fetchOnlineRegistry()
{
    if (m_registryUrl.isEmpty()) {
        return;
    }

    qDebug() << "Fetching MCP registry from:" << m_registryUrl;

    QNetworkRequest request(QUrl(m_registryUrl));
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                        QNetworkRequest::NoLessSafeRedirectPolicy);
    request.setHeader(QNetworkRequest::UserAgentHeader,
                     u"KDE Discover MCP Backend/1.0"_s);

    m_networkManager->get(request);
    m_fetchProgress = 30;
    Q_EMIT fetchingUpdatesProgressChanged();
}

void MCPBackend::onRegistryFetched(QNetworkReply *reply)
{
    reply->deleteLater();

    m_fetchProgress = 70;
    Q_EMIT fetchingUpdatesProgressChanged();

    if (reply->error() != QNetworkReply::NoError) {
        qWarning() << "Failed to fetch MCP registry:" << reply->errorString();
        m_fetching = false;
        m_fetchProgress = 100;
        Q_EMIT fetchingUpdatesProgressChanged();
        return;
    }

    const QByteArray data = reply->readAll();
    const QJsonDocument doc = QJsonDocument::fromJson(data);

    if (!doc.isObject()) {
        qWarning() << "Invalid MCP registry format";
        m_fetching = false;
        m_fetchProgress = 100;
        Q_EMIT fetchingUpdatesProgressChanged();
        return;
    }

    // Cache the registry data
    const QString cacheDir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
    QDir().mkpath(cacheDir);
    const QString cachePath = cacheDir + u"/mcp-registry.json"_s;

    QFile cacheFile(cachePath);
    if (cacheFile.open(QIODevice::WriteOnly)) {
        cacheFile.write(data);
        cacheFile.close();
    }

    // Parse the registry
    const QJsonArray servers = doc.object()[u"servers"_s].toArray();
    parseRegistryData(servers);

    m_fetching = false;
    m_fetchProgress = 100;
    Q_EMIT fetchingUpdatesProgressChanged();
    Q_EMIT contentsChanged();

    qDebug() << "Loaded" << servers.count() << "MCP servers from registry";
}

void MCPBackend::parseRegistryData(const QJsonArray &servers)
{
    for (const QJsonValue &serverValue : servers) {
        if (!serverValue.isObject()) {
            continue;
        }

        const QJsonObject serverObj = serverValue.toObject();
        const QString id = serverObj[u"id"_s].toString();

        // Check if we already have this resource (installed)
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

        // Create new resource
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
        // Resource already exists, don't overwrite
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
