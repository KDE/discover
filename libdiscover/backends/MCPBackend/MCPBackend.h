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
 * MCP servers are stored in:
 * - /usr/share/mcp/installed/ (system-wide)
 * - ~/.local/share/mcp/installed/ (user-specific)
 *
 * The registry catalog is fetched from configurable online sources.
 */
class MCPBackend : public AbstractResourcesBackend
{
    Q_OBJECT
    Q_PROPERTY(QString registryUrl READ registryUrl WRITE setRegistryUrl NOTIFY registryUrlChanged)

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
    QString registryUrl() const { return m_registryUrl; }
    void setRegistryUrl(const QString &url);

    // Get resource by ID
    MCPResource *resourceById(const QString &id) const;

Q_SIGNALS:
    void registryUrlChanged();

private Q_SLOTS:
    void onRegistryFetched(QNetworkReply *reply);

private:
    void loadInstalledServers();
    void loadRegistryCatalog();
    void fetchOnlineRegistry();
    void parseRegistryData(const QJsonArray &servers);
    void addResource(MCPResource *resource);
    MCPResource *createResourceFromJson(const QJsonObject &data);

    QHash<QString, MCPResource *> m_resources;
    StandardBackendUpdater *m_updater;
    QNetworkAccessManager *m_networkManager;

    QString m_registryUrl;
    bool m_fetching = false;
    int m_fetchProgress = 100;

    // Default registry URL - could be configured
    static constexpr const char *DEFAULT_REGISTRY_URL = "https://raw.githubusercontent.com/YakupAtahanov/mcp-registry/main/registry.json";
};
