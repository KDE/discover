/*
 *   SPDX-FileCopyrightText: 2012 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "PackageState.h"
#include "libdiscover_debug.h"

PackageState::PackageState(const QString &name, const QString &description, bool installed)
    : PackageState(name, name, description, installed)
{
}

PackageState::PackageState(QString packageName, QString name, QString description, bool installed)
    : m_packageName(std::move(packageName))
    , m_name(std::move(name))
    , m_description(std::move(description))
    , m_installed(installed)
{
}

QString PackageState::name() const
{
    return m_name;
}

QString PackageState::description() const
{
    return m_description;
}

QString PackageState::packageName() const
{
    return m_packageName;
}

bool PackageState::isInstalled() const
{
    return m_installed;
}

void PackageState::setInstalled(bool installed)
{
    m_installed = installed;
}

QDebug operator<<(QDebug debug, const PackageState &state)
{
    QDebugStateSaver saver(debug);
    debug.nospace() << "PackageState(";
    debug.nospace() << state.name() << ':';
    debug.nospace() << "installed: " << state.isInstalled() << ',';
    debug.nospace() << ')';
    return debug;
}
