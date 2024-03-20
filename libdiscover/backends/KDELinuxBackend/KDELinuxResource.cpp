// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2024 Harald Sitter <sitter@kde.org>

#include "KDELinuxResource.h"

#include <KLocalizedString>

using namespace Qt::StringLiterals;

KDELinuxResource::KDELinuxResource(const QString &version, AbstractResourcesBackend *parent)
    : AbstractResource(parent)
    , m_version(version)
{
}

QString KDELinuxResource::packageName() const
{
    return m_osrelease.id();
}

QString KDELinuxResource::name() const
{
    return m_osrelease.prettyName();
}

QString KDELinuxResource::comment()
{
    return {}; // Shown on Installed page as a subtext
}

QVariant KDELinuxResource::icon() const
{
    return m_osrelease.logo();
}

bool KDELinuxResource::canExecute() const
{
    return false;
}

AbstractResource::State KDELinuxResource::state()
{
    return Upgradeable;
}

bool KDELinuxResource::hasCategory(const QString &category) const
{
    return false;
}

AbstractResource::Type KDELinuxResource::type() const
{
    return AbstractResource::System;
}

quint64 KDELinuxResource::size()
{
    constexpr quint64 size = 5ULL * 1024 * 1024 * 1024; // We don't know the real update size, currently we are somewhere below 5GiB
    return size;
}

QJsonArray KDELinuxResource::licenses()
{
    return {};
}

QString KDELinuxResource::installedVersion() const
{
    return m_osrelease.versionId();
}

QString KDELinuxResource::availableVersion() const
{
    return m_version;
}

QString KDELinuxResource::longDescription()
{
    return {}; // Combined with the comment on the "app" page (e.g. clicking the entry on Installed page)
}

QString KDELinuxResource::origin() const
{
    return {};
}

QString KDELinuxResource::section()
{
    return u"section"_s;
}

QString KDELinuxResource::author() const
{
    return u"KDE"_s;
}

QList<PackageState> KDELinuxResource::addonsInformation()
{
    return {};
}

QString KDELinuxResource::sourceIcon() const
{
    return {};
}

QDate KDELinuxResource::releaseDate() const
{
    return {};
}

void KDELinuxResource::fetchChangelog()
{
}

void KDELinuxResource::invokeApplication() const
{
}
