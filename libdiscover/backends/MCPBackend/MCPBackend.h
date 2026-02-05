/*
 *   SPDX-FileCopyrightText: 2025 Yakup Atahanov
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include <resources/AbstractResourcesBackend.h>
#include <Category/Category.h>
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
 * MCP servers are stored in:
 * - /usr/share/mcp/installed/ (system-wide)
 * - ~/.local/share/mcp/installed/ (user-specific)
 *
 * Registry sources are configured in:
 * - ~/.config/mcp/sources.list (user)
 * - /etc/mcp/sources.list (system)
 *
 * Each line in sources.list is a URL to a registry JSON file.
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
    QList<std::shared_ptr<Category>> category() const override { return m_rootCategories; }
    InlineMessage *explainDysfunction() const override;

    // MCP-specific methods
    QStringList registrySources() const { return m_registrySources; }
    void addRegistrySource(const QString &url);
    void removeRegistrySource(const QString &url);

    // Get resource by ID
    MCPResource *resourceById(const QString &id) const;

Q_SIGNALS:
    void registrySourcesChanged();

private Q_SLOTS:
    void onRegistryFetched(QNetworkReply *reply);

private:
    void loadSourcesConfig();
    void saveSourcesConfig();
    void loadInstalledServers();
    void loadCachedRegistries();
    void fetchOnlineRegistries();
    void fetchNextRegistry();
    void parseRegistryData(const QJsonArray &servers);
    void addResource(MCPResource *resource);
    MCPResource *createResourceFromJson(const QJsonObject &data);
    void setupCategories();

    QHash<QString, MCPResource *> m_resources;
    QList<std::shared_ptr<Category>> m_rootCategories;
    StandardBackendUpdater *m_updater;
    QNetworkAccessManager *m_networkManager;

    QStringList m_registrySources;
    int m_currentFetchIndex = 0;
    bool m_fetching = false;
    int m_fetchProgress = 100;
};
