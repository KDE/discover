/*
 *   SPDX-FileCopyrightText: 2022 Jeremy Whiting <jeremy.whiting@collabora.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "SteamOSResource.h"
#include <QDesktopServices>
#include <QStringList>
#include <QTimer>
#include <Transaction/AddonList.h>

SteamOSResource::SteamOSResource(const QString &version, const QString &build, quint64 size, const QString &currentVersion, AbstractResourcesBackend *parent)
    : AbstractResource(parent)
    , m_name("SteamOS")
    , m_build(build)
    , m_version(version)
    , m_currentVersion(currentVersion)
    , m_appstreamId("steamos." + m_build)
    , m_state(State::Upgradeable)
    , m_addons()
    , m_type(AbstractResource::Technical)
    , m_size(size)
{
}

QString SteamOSResource::appstreamId() const
{
    return m_appstreamId;
}

QList<PackageState> SteamOSResource::addonsInformation()
{
    return m_addons;
}

QString SteamOSResource::availableVersion() const
{
    return QStringLiteral("%1 %2").arg(m_version, m_build);
}

QStringList SteamOSResource::categories()
{
    return {QStringLiteral("steamos")};
}

QString SteamOSResource::comment()
{
    return name();
}

quint64 SteamOSResource::size()
{
    return m_size;
}

QUrl SteamOSResource::homepage()
{
    return QUrl(QStringLiteral("https://store.steampowered.com/"));
}

QUrl SteamOSResource::helpURL()
{
    return QUrl(QStringLiteral("https://store.steampowered.com/"));
}

QUrl SteamOSResource::bugURL()
{
    return QUrl(QStringLiteral("https://steamcommunity.com/app/1675200/discussions/"));
}

QUrl SteamOSResource::donationURL()
{
    return {};
}

QUrl SteamOSResource::contributeURL()
{
    return {};
}

QVariant SteamOSResource::icon() const
{
    return QStringLiteral("steam");
}

QString SteamOSResource::installedVersion() const
{
    return m_currentVersion;
}

QJsonArray SteamOSResource::licenses()
{
    return {};
}

QString SteamOSResource::longDescription()
{
    return {};
}

QString SteamOSResource::name() const
{
    return m_name;
}

QString SteamOSResource::origin() const
{
    return QStringLiteral("SteamOS");
}

QString SteamOSResource::packageName() const
{
    return m_name;
}

bool SteamOSResource::isRemovable() const
{
    return false;
}

AbstractResource::Type SteamOSResource::type() const
{
    return m_type;
}

bool SteamOSResource::canExecute() const
{
    return false;
}

QString SteamOSResource::section()
{
    return QStringLiteral("SteamOS");
}

AbstractResource::State SteamOSResource::state()
{
    return m_state;
}

void SteamOSResource::fetchChangelog()
{
    QString log = longDescription();
    log.replace(QLatin1Char('\n'), QLatin1String("<br />"));

    Q_EMIT changelogFetched(log);
}

void SteamOSResource::fetchScreenshots()
{
    QList<QUrl> empty;
    Q_EMIT screenshotsFetched(empty, empty);
}

void SteamOSResource::setState(AbstractResource::State state)
{
    if (m_state == state)
        return;

    m_state = state;
    Q_EMIT stateChanged();
}

void SteamOSResource::setSize(quint64 size)
{
    m_size = size;
    Q_EMIT sizeChanged();
}

QString SteamOSResource::sourceIcon() const
{
    return QStringLiteral("steam");
}

QDate SteamOSResource::releaseDate() const
{
    return {};
}

void SteamOSResource::setVersion(const QString &version)
{
    m_version = version;
}

void SteamOSResource::setBuild(const QString &build)
{
    m_build = build;
    m_appstreamId = "steamos." + m_build;
}

QString SteamOSResource::getBuild() const
{
    return m_build;
}

QUrl SteamOSResource::url() const
{
    return QUrl(QLatin1String("steamos://") + packageName().replace(QLatin1Char(' '), QLatin1Char('.')));
}

QString SteamOSResource::author() const
{
    return QStringLiteral("Valve");
}
