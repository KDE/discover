/***************************************************************************
 *   Copyright © 2011 Jonathan Thomas <echidnaman@kubuntu.org>             *
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

#include "UpdateItem.h"
#include <resources/AbstractResource.h>
#include <resources/AbstractResourcesBackend.h>
#include <resources/AbstractBackendUpdater.h>

#include <QtCore/QStringBuilder>
#include <KLocalizedString>
#include <QDebug>

UpdateItem::UpdateItem(AbstractResource *app)
    : m_app(app)
    , m_progress(0)
{
}

UpdateItem::~UpdateItem()
{
}

AbstractResource *UpdateItem::app() const
{
    return m_app;
}

QString UpdateItem::name() const
{
    return m_app->name();
}

QString UpdateItem::version() const
{
    return m_app->availableVersion();
}

QIcon UpdateItem::icon() const
{
    return QIcon::fromTheme(m_app->icon());
}

qint64 UpdateItem::size() const
{
    return m_app->size();
}

static bool isMarked(AbstractResource* res)
{
    return res->backend()->backendUpdater()->isMarked(res);
}

Qt::CheckState UpdateItem::checked() const
{
    return isMarked(app()) ? Qt::Checked : Qt::Unchecked;
}

qreal UpdateItem::progress() const
{
    return m_progress;
}

void UpdateItem::setProgress(qreal progress)
{
    m_progress = progress;
}

QString UpdateItem::changelog() const
{
    return m_changelog;
}

void UpdateItem::setChangelog(const QString& changelog)
{
    m_changelog = changelog;
}
