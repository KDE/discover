/*
 *   SPDX-FileCopyrightText: 2025 Yakup Atahanov
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include <resources/AbstractResource.h>
#include <QJsonObject>

class MCPBackend;

/**
 * \class MCPResource MCPResource.h
 *
 * \brief Represents an MCP (Model Context Protocol) server resource.
 *
 * MCP servers provide tools and capabilities for LLMs. This resource class
 * represents a single MCP server that can be installed, removed, and configured
 * through KDE Discover.
 */
class MCPResource : public AbstractResource
{
    Q_OBJECT
    Q_PROPERTY(QString transportType READ transportType CONSTANT)
    Q_PROPERTY(QStringList capabilities READ capabilities CONSTANT)
    Q_PROPERTY(QStringList permissions READ permissions CONSTANT)

public:
    /**
     * Transport types for MCP servers
     */
    enum class TransportType {
        Stdio,      ///< Local process via stdin/stdout
        SSE,        ///< Server-Sent Events (HTTP)
        WebSocket,  ///< WebSocket connection
    };
    Q_ENUM(TransportType)

    /**
     * Installation source types
     */
    enum class SourceType {
        Npm,        ///< npm package
        Pip,        ///< Python pip package
        Binary,     ///< Direct binary download
        Container,  ///< Container image (podman/docker)
        Git,        ///< Git repository
    };
    Q_ENUM(SourceType)

    explicit MCPResource(const QJsonObject &data, MCPBackend *parent);

    // AbstractResource interface
    QString packageName() const override;
    QString name() const override;
    QString comment() override;
    QVariant icon() const override;
    bool canExecute() const override;
    void invokeApplication() const override;
    State state() override;
    bool hasCategory(const QString &category) const override;
    QUrl homepage() override;
    QUrl bugURL() override;
    QUrl donationURL() override;
    QJsonArray licenses() override;
    QString longDescription() override;
    QString installedVersion() const override;
    QString availableVersion() const override;
    QString origin() const override;
    QString section() override;
    QString author() const override;
    quint64 size() override;
    QList<PackageState> addonsInformation() override;
    Type type() const override;
    QString sourceIcon() const override;
    QDate releaseDate() const override;
    void fetchChangelog() override;
    void fetchScreenshots() override;
    QUrl url() const override;

    // MCP-specific properties
    QString transportType() const;
    TransportType transport() const { return m_transport; }
    SourceType sourceType() const { return m_sourceType; }
    QStringList capabilities() const { return m_capabilities; }
    QStringList permissions() const { return m_permissions; }
    QStringList tools() const { return m_tools; }
    QString command() const { return m_command; }
    QStringList args() const { return m_args; }
    QString sseUrl() const { return m_sseUrl; }

    // State management
    void setState(State state);
    void setInstalledVersion(const QString &version);

    // Get the raw JSON data for this server
    QJsonObject toJson() const;

    // Get the manifest path for installed servers
    QString manifestPath() const;

private:
    void parseJsonData(const QJsonObject &data);

    QString m_id;
    QString m_name;
    QString m_summary;
    QString m_description;
    QString m_version;
    QString m_installedVersion;
    QString m_author;
    QString m_homepage;
    QString m_bugUrl;
    QString m_donationUrl;
    QString m_license;
    QString m_licenseUrl;
    QString m_iconName;
    QString m_changelog;
    QDate m_releaseDate;
    quint64 m_size = 0;

    TransportType m_transport = TransportType::Stdio;
    SourceType m_sourceType = SourceType::Npm;

    // Transport configuration
    QString m_command;           // For stdio transport
    QStringList m_args;          // For stdio transport
    QString m_sseUrl;            // For SSE transport
    QString m_wsUrl;             // For WebSocket transport

    // Source configuration
    QString m_sourcePackage;     // npm package name, pip package, etc.
    QString m_sourceUrl;         // Download URL for binary/git

    QStringList m_categories;
    QStringList m_capabilities;
    QStringList m_permissions;
    QStringList m_tools;
    Screenshots m_screenshots;

    State m_state = State::None;
};
