/*
 *   SPDX-FileCopyrightText: 2025 Yakup Atahanov
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "MCPResource.h"
#include "MCPBackend.h"

#include <QDesktopServices>
#include <QDir>
#include <QFileInfo>
#include <QJsonArray>
#include <QStandardPaths>

using namespace Qt::StringLiterals;

MCPResource::MCPResource(const QJsonObject &data, MCPBackend *parent)
    : AbstractResource(parent)
{
    parseJsonData(data);
}

void MCPResource::parseJsonData(const QJsonObject &data)
{
    m_id = data[u"id"_s].toString();
    m_name = data[u"name"_s].toString();
    m_summary = data[u"summary"_s].toString();
    m_description = data[u"description"_s].toString();
    m_version = data[u"version"_s].toString();
    m_author = data[u"author"_s].toString();
    m_homepage = data[u"homepage"_s].toString();
    m_bugUrl = data[u"bugUrl"_s].toString();
    m_donationUrl = data[u"donationUrl"_s].toString();
    m_iconName = data[u"icon"_s].toString(u"application-x-executable"_s);
    m_size = data[u"size"_s].toVariant().toULongLong();

    // Parse license
    const auto licenseObj = data[u"license"_s].toObject();
    m_license = licenseObj[u"name"_s].toString(u"Unknown"_s);
    m_licenseUrl = licenseObj[u"url"_s].toString();

    // Parse release date
    const QString releaseDateStr = data[u"releaseDate"_s].toString();
    if (!releaseDateStr.isEmpty()) {
        m_releaseDate = QDate::fromString(releaseDateStr, Qt::ISODate);
    }

    // Parse transport type
    const QString transportStr = data[u"type"_s].toString(u"stdio"_s);
    if (transportStr == u"stdio"_s) {
        m_transport = TransportType::Stdio;
    } else if (transportStr == u"sse"_s) {
        m_transport = TransportType::SSE;
    } else if (transportStr == u"websocket"_s) {
        m_transport = TransportType::WebSocket;
    }

    // Parse transport configuration
    const auto transportObj = data[u"transport"_s].toObject();
    m_command = transportObj[u"command"_s].toString();
    const auto argsArray = transportObj[u"args"_s].toArray();
    for (const auto &arg : argsArray) {
        m_args.append(arg.toString());
    }
    m_sseUrl = transportObj[u"url"_s].toString();
    m_wsUrl = transportObj[u"wsUrl"_s].toString();

    // Parse source configuration
    const auto sourceObj = data[u"source"_s].toObject();
    const QString sourceTypeStr = sourceObj[u"type"_s].toString(u"npm"_s);
    if (sourceTypeStr == u"npm"_s) {
        m_sourceType = SourceType::Npm;
    } else if (sourceTypeStr == u"pip"_s) {
        m_sourceType = SourceType::Pip;
    } else if (sourceTypeStr == u"binary"_s) {
        m_sourceType = SourceType::Binary;
    } else if (sourceTypeStr == u"container"_s) {
        m_sourceType = SourceType::Container;
    } else if (sourceTypeStr == u"git"_s) {
        m_sourceType = SourceType::Git;
    }
    m_sourcePackage = sourceObj[u"package"_s].toString();
    m_sourceUrl = sourceObj[u"url"_s].toString();

    // Parse categories
    const auto categoriesArray = data[u"categories"_s].toArray();
    for (const auto &cat : categoriesArray) {
        m_categories.append(cat.toString());
    }

    // Parse capabilities
    const auto capabilitiesArray = data[u"capabilities"_s].toArray();
    for (const auto &cap : capabilitiesArray) {
        m_capabilities.append(cap.toString());
    }

    // Parse permissions
    const auto permissionsArray = data[u"permissions"_s].toArray();
    for (const auto &perm : permissionsArray) {
        m_permissions.append(perm.toString());
    }

    // Parse tools
    const auto toolsArray = data[u"tools"_s].toArray();
    for (const auto &tool : toolsArray) {
        if (tool.isString()) {
            m_tools.append(tool.toString());
        } else if (tool.isObject()) {
            m_tools.append(tool.toObject()[u"name"_s].toString());
        }
    }

    // Parse screenshots
    const auto screenshotsArray = data[u"screenshots"_s].toArray();
    for (const auto &screenshot : screenshotsArray) {
        if (screenshot.isString()) {
            m_screenshots.append(Screenshot(QUrl(screenshot.toString())));
        } else if (screenshot.isObject()) {
            const auto ssObj = screenshot.toObject();
            m_screenshots.append(Screenshot(
                QUrl(ssObj[u"thumbnail"_s].toString()),
                QUrl(ssObj[u"url"_s].toString()),
                ssObj[u"animated"_s].toBool(false),
                QSize()
            ));
        }
    }

    // Parse changelog
    m_changelog = data[u"changelog"_s].toString();

    // Parse required properties
    const auto propsArray = data[u"requiredProperties"_s].toArray();
    for (const auto &prop : propsArray) {
        const auto propObj = prop.toObject();
        RequiredProperty propDef;
        propDef.key = propObj[u"key"_s].toString();
        propDef.label = propObj[u"label"_s].toString();
        propDef.description = propObj[u"description"_s].toString();
        propDef.sensitive = propObj[u"sensitive"_s].toBool(false);
        propDef.required = propObj[u"required"_s].toBool(true);
        m_requiredProperties.append(propDef);
    }

    // Load existing property values from config object (for installed servers)
    const auto configObj = data[u"config"_s].toObject();
    for (auto it = configObj.begin(); it != configObj.end(); ++it) {
        m_propertyValues[it.key()] = it.value().toString();
    }
}

QString MCPResource::packageName() const
{
    return m_id;
}

QString MCPResource::name() const
{
    return m_name;
}

QString MCPResource::comment()
{
    return m_summary;
}

QVariant MCPResource::icon() const
{
    return m_iconName;
}

bool MCPResource::canExecute() const
{
    // MCP servers can be "executed" (tested/configured) if installed
    return m_state == State::Installed || m_state == State::Upgradeable;
}

void MCPResource::invokeApplication() const
{
    // Open configuration or documentation for the MCP server
    if (!m_homepage.isEmpty()) {
        QDesktopServices::openUrl(QUrl(m_homepage));
    }
}

AbstractResource::State MCPResource::state()
{
    return m_state;
}

bool MCPResource::hasCategory(const QString &category) const
{
    // Always match "mcp" category for all MCP servers
    if (category == u"mcp"_s) {
        return true;
    }
    return m_categories.contains(category, Qt::CaseInsensitive);
}

QUrl MCPResource::homepage()
{
    return QUrl(m_homepage);
}

QUrl MCPResource::bugURL()
{
    return QUrl(m_bugUrl);
}

QUrl MCPResource::donationURL()
{
    return QUrl(m_donationUrl);
}

QJsonArray MCPResource::licenses()
{
    return {QJsonObject{
        {u"name"_s, m_license},
        {u"url"_s, m_licenseUrl}
    }};
}

QString MCPResource::longDescription()
{
    QString desc = m_description;

    // Append tools information if available
    if (!m_tools.isEmpty()) {
        desc += u"\n\n**Available Tools:**\n"_s;
        for (const auto &tool : m_tools) {
            desc += u"- "_s + tool + u"\n"_s;
        }
    }

    // Append capabilities
    if (!m_capabilities.isEmpty()) {
        desc += u"\n**Capabilities:**\n"_s;
        for (const auto &cap : m_capabilities) {
            desc += u"- "_s + cap + u"\n"_s;
        }
    }

    // Append permissions required
    if (!m_permissions.isEmpty()) {
        desc += u"\n**Required Permissions:**\n"_s;
        for (const auto &perm : m_permissions) {
            desc += u"- "_s + perm + u"\n"_s;
        }
    }

    return desc;
}

QString MCPResource::installedVersion() const
{
    return m_installedVersion;
}

QString MCPResource::availableVersion() const
{
    return m_version;
}

QString MCPResource::origin() const
{
    return u"MCP Registry"_s;
}

QString MCPResource::section()
{
    return u"mcp"_s;
}

QString MCPResource::author() const
{
    return m_author;
}

quint64 MCPResource::size()
{
    return m_size;
}

QList<PackageState> MCPResource::addonsInformation()
{
    // MCP servers don't have addons in the traditional sense
    return {};
}

AbstractResource::Type MCPResource::type() const
{
    return AbstractResource::Application;
}

QString MCPResource::sourceIcon() const
{
    // Return an icon based on the source type
    switch (m_sourceType) {
    case SourceType::Npm:
        return u"application-javascript"_s;
    case SourceType::Pip:
        return u"text-x-python"_s;
    case SourceType::Container:
        return u"application-x-iso9660-appimage"_s;
    case SourceType::Git:
        return u"git"_s;
    case SourceType::Binary:
    default:
        return u"application-x-executable"_s;
    }
}

QDate MCPResource::releaseDate() const
{
    return m_releaseDate;
}

void MCPResource::fetchChangelog()
{
    Q_EMIT changelogFetched(m_changelog);
}

void MCPResource::fetchScreenshots()
{
    Q_EMIT screenshotsFetched(m_screenshots);
}

QUrl MCPResource::url() const
{
    return QUrl(u"mcp://"_s + m_id);
}

QString MCPResource::transportType() const
{
    switch (m_transport) {
    case TransportType::Stdio:
        return u"stdio"_s;
    case TransportType::SSE:
        return u"sse"_s;
    case TransportType::WebSocket:
        return u"websocket"_s;
    }
    return u"unknown"_s;
}

void MCPResource::setState(State state)
{
    if (m_state != state) {
        m_state = state;
        Q_EMIT stateChanged();
    }
}

void MCPResource::setInstalledVersion(const QString &version)
{
    m_installedVersion = version;
}

QJsonObject MCPResource::toJson() const
{
    QJsonObject obj;
    obj[u"id"_s] = m_id;
    obj[u"name"_s] = m_name;
    obj[u"summary"_s] = m_summary;
    obj[u"description"_s] = m_description;
    obj[u"version"_s] = m_version;
    obj[u"author"_s] = m_author;
    obj[u"homepage"_s] = m_homepage;
    obj[u"icon"_s] = m_iconName;

    // Transport type
    obj[u"type"_s] = transportType();

    // Transport configuration
    QJsonObject transportObj;
    if (m_transport == TransportType::Stdio) {
        transportObj[u"command"_s] = m_command;
        QJsonArray argsArray;
        for (const QString &arg : m_args) {
            argsArray.append(arg);
        }
        transportObj[u"args"_s] = argsArray;
    } else if (m_transport == TransportType::SSE) {
        transportObj[u"url"_s] = m_sseUrl;
    }
    obj[u"transport"_s] = transportObj;

    // Source
    QJsonObject sourceObj;
    switch (m_sourceType) {
    case SourceType::Npm:
        sourceObj[u"type"_s] = u"npm"_s;
        break;
    case SourceType::Pip:
        sourceObj[u"type"_s] = u"pip"_s;
        break;
    case SourceType::Binary:
        sourceObj[u"type"_s] = u"binary"_s;
        break;
    case SourceType::Container:
        sourceObj[u"type"_s] = u"container"_s;
        break;
    case SourceType::Git:
        sourceObj[u"type"_s] = u"git"_s;
        break;
    }
    sourceObj[u"package"_s] = m_sourcePackage;
    if (!m_sourceUrl.isEmpty()) {
        sourceObj[u"url"_s] = m_sourceUrl;
    }
    obj[u"source"_s] = sourceObj;

    // Convert QStringLists to QJsonArrays manually for Qt compatibility
    QJsonArray categoriesArray, capabilitiesArray, permissionsArray, toolsArray;
    for (const QString &cat : m_categories) categoriesArray.append(cat);
    for (const QString &cap : m_capabilities) capabilitiesArray.append(cap);
    for (const QString &perm : m_permissions) permissionsArray.append(perm);
    for (const QString &tool : m_tools) toolsArray.append(tool);

    obj[u"categories"_s] = categoriesArray;
    obj[u"capabilities"_s] = capabilitiesArray;
    obj[u"permissions"_s] = permissionsArray;
    obj[u"tools"_s] = toolsArray;

    // Include required properties definition
    if (!m_requiredProperties.isEmpty()) {
        QJsonArray propsArray;
        for (const auto &prop : m_requiredProperties) {
            QJsonObject propObj;
            propObj[u"key"_s] = prop.key;
            propObj[u"label"_s] = prop.label;
            propObj[u"description"_s] = prop.description;
            propObj[u"sensitive"_s] = prop.sensitive;
            propObj[u"required"_s] = prop.required;
            propsArray.append(propObj);
        }
        obj[u"requiredProperties"_s] = propsArray;
    }

    // Include configuration values
    if (!m_propertyValues.isEmpty()) {
        QJsonObject configObj;
        for (auto it = m_propertyValues.begin(); it != m_propertyValues.end(); ++it) {
            configObj[it.key()] = it.value();
        }
        obj[u"config"_s] = configObj;
    }

    return obj;
}

QString MCPResource::manifestPath() const
{
    // Installed server manifests go to /usr/share/mcp/installed/ (system-wide)
    // or ~/.local/share/mcp/installed/ (user-specific)

    // Try system directory first
    const QString systemDirPath = u"/usr/share/mcp/installed/"_s;
    const QDir systemDir(systemDirPath);

    // Check if directory exists and is writable, or if parent (/usr/share) is writable (root)
    if (systemDir.exists()) {
        QFileInfo dirInfo(systemDirPath);
        if (dirInfo.isWritable()) {
            return systemDirPath + m_id + u".json"_s;
        }
    }
    // Also check if /usr/share is writable (running as root)
    QFileInfo usrShareInfo(u"/usr/share"_s);
    if (usrShareInfo.isWritable()) {
        return systemDirPath + m_id + u".json"_s;
    }

    // Fall back to user directory
    const QString userDataDir = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation);
    return userDataDir + u"/mcp/installed/"_s + m_id + u".json"_s;
}

void MCPResource::setPropertyValue(const QString &key, const QString &value)
{
    if (m_propertyValues.value(key) != value) {
        m_propertyValues[key] = value;
        Q_EMIT propertyValuesChanged();
    }
}

QString MCPResource::propertyValue(const QString &key) const
{
    return m_propertyValues.value(key);
}

QVariantList MCPResource::requiredPropertiesQml() const
{
    QVariantList result;
    for (const auto &prop : m_requiredProperties) {
        QVariantMap propMap;
        propMap[u"key"_s] = prop.key;
        propMap[u"label"_s] = prop.label;
        propMap[u"description"_s] = prop.description;
        propMap[u"sensitive"_s] = prop.sensitive;
        propMap[u"required"_s] = prop.required;
        result.append(propMap);
    }
    return result;
}

QVariantMap MCPResource::propertyValuesQml() const
{
    QVariantMap result;
    for (auto it = m_propertyValues.begin(); it != m_propertyValues.end(); ++it) {
        result[it.key()] = it.value();
    }
    return result;
}

#include "moc_MCPResource.cpp"
