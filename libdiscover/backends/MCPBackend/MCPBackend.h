/*
 *   SPDX-FileCopyrightText: 2025 Yakup Atahanov
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include <resources/AbstractResourcesBackend.h>
#include <QHash>
#include <QJsonArray>

class MCPResource;
class StandardBackendUpdater;
class QNetworkAccessManager;
class QNetworkReply;

/**
 * \class MCPBackend MCPBackend.h
 *
 * \brief Backend for MCP (Model Context Protocol) server management.
 *
 * This backend allows users to discover, install, and manage MCP servers
 * through KDE Discover. It integrates with Project JARVIS's SuperMCP
 * orchestration layer by providing a system-wide MCP server registry.
 *
 * System-managed storage (requires privilege escalation via pkexec):
 * - /usr/share/mcp/installed/{id}/           (per-server directory with files and manifest)
 * - /usr/share/mcp/installed/{id}/manifest.json  (server metadata, source of truth)
 * - /usr/share/mcp/mcp.json                  (index of all installed servers, kept in sync)
 *
 * User-managed storage (no privilege escalation needed):
 * - ~/.config/mcp/config.json          (user-specific config: API keys, etc.)
 * - ~/.config/mcp/sources.list         (registry source URLs)
 *
 * Registry sources (system-wide, optional):
 * - /etc/mcp/sources.list
 */
class MCPBackend : public AbstractResourcesBackend
{
    Q_OBJECT
    Q_PROPERTY(QStringList registrySources READ registrySources NOTIFY registrySourcesChanged)

public:
    explicit MCPBackend(QObject *parent = nullptr);
    ~MCPBackend() override;

    // AbstractResourcesBackend interface
    bool isValid() const override { return true; }
    QString displayName() const override;
    int updatesCount() const override;
    int fetchingUpdatesProgress() const override;
    AbstractBackendUpdater *backendUpdater() const override;
    AbstractReviewsBackend *reviewsBackend() const override;
    ResultsStream *search(const AbstractResourcesBackend::Filters &filter) override;
    Transaction *installApplication(AbstractResource *app, const AddonList &addons) override;
    Transaction *installApplication(AbstractResource *app) override;
    Transaction *removeApplication(AbstractResource *app) override;
    void checkForUpdates() override;
    bool hasApplications() const override { return true; }
    InlineMessage *explainDysfunction() const override;

    // MCP-specific methods
    QStringList registrySources() const { return m_registrySources; }
    void addRegistrySource(const QString &url);
    void removeRegistrySource(const QString &url);

    // Get resource by ID
    MCPResource *resourceById(const QString &id) const;

    /// Add an entry to the system mcp.json (requires pkexec)
    bool addServerToMcpJson(const QJsonObject &serverData);
    /// Remove an entry from the system mcp.json (requires pkexec)
    bool removeServerFromMcpJson(const QString &serverId);

    /// Write per-server manifest to installed/{id}/manifest.json (requires pkexec)
    bool writeServerManifest(const QString &serverId, const QJsonObject &manifest);

    /// System paths
    static QString systemMcpDir();
    static QString systemMcpJsonPath();
    static QString systemInstalledDir();
    static QString serverInstallDir(const QString &serverId);
    static QString serverManifestPath(const QString &serverId);

    /// Default registry URL (always included)
    static QString defaultRegistryUrl();

Q_SIGNALS:
    void registrySourcesChanged();

private Q_SLOTS:
    void onRegistryFetched(QNetworkReply *reply);

private:
    void bootstrapUserConfig();
    void loadSourcesConfig();
    void saveSourcesConfig();
    void loadInstalledServers();
    void loadMcpJson();
    void loadCachedRegistries();
    void fetchOnlineRegistries();
    void fetchNextRegistry();
    void parseRegistryData(const QJsonArray &servers);
    void addResource(MCPResource *resource);
    MCPResource *createResourceFromJson(const QJsonObject &data);

    /// Execute a privileged write operation via pkexec
    bool privilegedWriteFile(const QString &filePath, const QByteArray &content);

    QHash<QString, MCPResource *> m_resources;
    StandardBackendUpdater *m_updater;
    QNetworkAccessManager *m_networkManager;

    QStringList m_registrySources;
    int m_currentFetchIndex = 0;
    bool m_fetching = false;
    int m_fetchProgress = 100;
};
