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
#include "SnapBackend.h"
#include <QDebug>
#include <QProcess>

SnapResource::SnapResource(QJsonObject data, AbstractResource::State state, AbstractResourcesBackend* parent)
    : AbstractResource(parent)
    , m_state(state)
    , m_data(std::move(data))
{
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
    return m_data.value(QLatin1String("summary")).toString();
}

int SnapResource::size()
{
    return m_data.value(QLatin1String("installed-size")).toInt();
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
    return m_data.value(QLatin1String("version")).toString();
}

QString SnapResource::license()
{
    return {};
}

QString SnapResource::longDescription()
{
    return m_data.value(QLatin1String("description")).toString();
}

QString SnapResource::name()
{
    return m_data.value(QLatin1String("name")).toString();
}

QString SnapResource::origin() const
{
    return QStringLiteral("snappy:") + m_data.value(QLatin1String("channel")).toString();
}

QString SnapResource::packageName() const
{
    return m_data.value(QLatin1String("id")).toString();
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
    return m_state;
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
    QProcess::startDetached(m_data[QLatin1String("resource")].toString());
}

bool SnapResource::isTechnical() const
{
    return m_data.value(QLatin1String("type")) == QLatin1String("os") || m_data.value(QLatin1String("private")).toBool();
}

void SnapResource::refreshState()
{
    auto b = qobject_cast<SnapBackend*>(backend());
    SnapSocket* socket = b->socket();
    auto job = socket->snapByName(packageName());
    connect(job, &SnapJob::finished, this, [this](SnapJob* job){
        m_state = job->isSuccessful() ? AbstractResource::Installed : AbstractResource::None;
        Q_EMIT stateChanged();
        qDebug() << "refreshed!!" << job->result() << job->statusCode() << m_state;
    });
}
