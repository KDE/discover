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
#include <QBuffer>
#include <QImageReader>

SnapResource::SnapResource(QSharedPointer<QSnapdSnap> snap, AbstractResource::State state, SnapBackend* parent)
    : AbstractResource(parent)
    , m_state(state)
    , m_snap(snap)
{
    setObjectName(snap->name());
}

QString SnapResource::availableVersion() const
{
    return installedVersion();
}

QStringList SnapResource::categories()
{
    return { QStringLiteral("Application") };
}

QString SnapResource::comment()
{
    return m_snap->summary();
}

int SnapResource::size()
{
//     return isInstalled() ? m_snap->installedSize() : m_snap->downloadSize();
    return m_snap->downloadSize();
}

QUrl SnapResource::homepage()
{
    return {};
}

QVariant SnapResource::icon() const
{
    if (m_icon.isNull()) {
        m_icon = [this]() -> QVariant {
            const auto iconPath = m_snap->icon();
            if (iconPath.isEmpty())
                return QStringLiteral("package-x-generic");

            if (!iconPath.startsWith(QLatin1Char('/')))
                return QUrl(iconPath);

            auto backend = qobject_cast<SnapBackend*>(parent());
            auto req = backend->client()->getIcon(packageName());
            connect(req, &QSnapdGetIconRequest::complete, this, &SnapResource::gotIcon);
            req->runAsync();
            return {};
        }();
    }
    return m_icon;
}

void SnapResource::gotIcon()
{
    auto req = qobject_cast<QSnapdGetIconRequest*>(sender());
    if (req->error()) {
        qWarning() << "icon error" << req->errorString();
        return;
    }

    auto icon = req->icon();

    QBuffer buffer;
    buffer.setData(icon->data());
    QImageReader reader(&buffer);

    auto theIcon = QVariant::fromValue<QImage>(reader.read());
    if (theIcon != m_icon) {
        m_icon = theIcon;
        iconChanged();
    }
}

QString SnapResource::installedVersion() const
{
    return m_snap->version();
}

QString SnapResource::license()
{
    return m_snap->license();
}

QString SnapResource::longDescription()
{
    return m_snap->description();
}

QString SnapResource::name()
{
    return m_snap->title().isEmpty() ? m_snap->name() : m_snap->title();
}

QString SnapResource::origin() const
{
    return QStringLiteral("snappy:") + m_snap->channel();
}

QString SnapResource::packageName() const
{
    return m_snap->name();
}

QString SnapResource::section()
{
    return QStringLiteral("snap");
}

AbstractResource::State SnapResource::state() const
{
    return m_state;
}

void SnapResource::setState(AbstractResource::State state)
{
    if (m_state != state) {
        m_state = state;
        Q_EMIT stateChanged();
    }
}

void SnapResource::fetchChangelog()
{
    QString log;
    emit changelogFetched(log);
}

void SnapResource::fetchScreenshots()
{
    QList<QUrl> screenshots;
    for(int i = 0, c = m_snap->screenshotCount(); i<c; ++i) {
        QScopedPointer<QSnapdScreenshot> screenshot(m_snap->screenshot(i));
        screenshots << QUrl(screenshot->url());
    }
    Q_EMIT screenshotsFetched(screenshots, screenshots);
}

void SnapResource::invokeApplication() const
{
//     QProcess::startDetached(m_snap->price());
}

bool SnapResource::isTechnical() const
{
    return m_snap->snapType() != QLatin1String("app");
}

QUrl SnapResource::url() const
{
    //FIXME interim, until it has an appstreamId
    return QUrl(QStringLiteral("snap://") + packageName());
}

void SnapResource::setSnap(const QSharedPointer<QSnapdSnap>& snap)
{
    Q_ASSERT(snap->name() == m_snap->name());
    if (m_snap == snap)
        return;

    const bool newSize = m_snap->installedSize() != snap->installedSize() || m_snap->downloadSize() != snap->downloadSize();
    m_snap = snap;
    if (newSize)
        Q_EMIT sizeChanged();
}
