/***************************************************************************
 *   Copyright © 2013 Aleix Pol Gonzalez <aleixpol@blue-systems.com>       *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU General Public License as        *
 *   published by the Free Software Foundation; either version 2 of        *
 *   the License or (at your option) version 3 or any later version        *
 *   accepted by the membership of KDE e.V. (or its successor approved     *
 *   by the membership of KDE e.V.), which shall act as a proxy            *
 *   defined in Section 14 of version 3 of the license.                    *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>. *
 ***************************************************************************/

#include "SnapResource.h"
#include <QDebug>

SnapResource::SnapResource(QJsonObject data, AbstractResourcesBackend* parent)
    : AbstractResource(parent)
    , m_data(std::move(data))
{
    qDebug() << "fuuuuuuuu" << m_data;
}

QString SnapResource::availableVersion() const
{
    return {};
}

QStringList SnapResource::categories()
{
    return {};
}

QString SnapResource::comment()
{
    return {};
}

int SnapResource::size()
{
    return 0;
}

QUrl SnapResource::homepage()
{
    return {};
}

QVariant SnapResource::icon() const
{
    return {};
}

QString SnapResource::installedVersion() const
{
    return {};
}

QString SnapResource::license()
{
    return {};
}

QString SnapResource::longDescription()
{
    return {};
}

QString SnapResource::name()
{
    return m_data.value(QLatin1String("name")).toString();
}

QString SnapResource::origin() const
{
    return QStringLiteral("Snappy");
}

QString SnapResource::packageName() const
{
    return {};
}

QUrl SnapResource::screenshotUrl()
{
    return {};
}

QUrl SnapResource::thumbnailUrl()
{
    return {};
}

QString SnapResource::section()
{
    return QStringLiteral("snap");
}

AbstractResource::State SnapResource::state()
{
    return None;
}

void SnapResource::fetchChangelog()
{
    QString log;
    emit changelogFetched(log);
}

void SnapResource::fetchScreenshots()
{
    Q_EMIT screenshotsFetched({}, {});
}

void SnapResource::invokeApplication() const
{
    //TODO
}
