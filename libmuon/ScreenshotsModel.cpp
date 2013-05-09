/***************************************************************************
 *   Copyright © 2012 Aleix Pol Gonzalez <aleixpol@blue-systems.com>       *
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

#include "ScreenshotsModel.h"
#include <resources/AbstractResource.h>
// #include <tests/modeltest.h>
#include <KDebug>

ScreenshotsModel::ScreenshotsModel(QObject* parent)
    : QAbstractListModel(parent)
    , m_resource(0)
{
    QHash<int, QByteArray> roles = roleNames();
    roles.insert(ThumbnailUrl, "small_image_url");
    roles.insert(ScreenshotUrl, "large_image_url");
    setRoleNames(roles);
}

void ScreenshotsModel::setResource(AbstractResource* res)
{
    if(res == m_resource)
        return;

    if(m_resource) {
        disconnect(m_resource, SIGNAL(screenshotsFetched(QList<QUrl>,QList<QUrl>)), this,
                                SLOT(screenshotsFetched(QList<QUrl>,QList<QUrl>)));
    }
    m_resource = res;
    
    if(res) {
        connect(m_resource, SIGNAL(screenshotsFetched(QList<QUrl>,QList<QUrl>)),
                        SLOT(screenshotsFetched(QList<QUrl>,QList<QUrl>)));
        res->fetchScreenshots();
    } else
        kDebug() << "empty resource!";
}

AbstractResource* ScreenshotsModel::resource() const
{
    return m_resource;
}

void ScreenshotsModel::screenshotsFetched(const QList< QUrl >& thumbnails, const QList< QUrl >& screenshots)
{
    Q_ASSERT(thumbnails.count()==screenshots.count());
    if (thumbnails.size() == 0)
        return;
    
    beginInsertRows(QModelIndex(), m_thumbnails.size(), m_thumbnails.size()+thumbnails.size()-1);
    m_thumbnails += thumbnails;
    m_screenshots += screenshots;
    endInsertRows();
    emit countChanged();
}

QVariant ScreenshotsModel::data(const QModelIndex& index, int role) const
{
    if(!index.isValid() || index.parent().isValid())
        return QVariant();
    
    switch(role) {
        case ThumbnailUrl: return m_thumbnails[index.row()];
        case ScreenshotUrl: return m_screenshots[index.row()];
    }
    
    return QVariant();
}

int ScreenshotsModel::rowCount(const QModelIndex& parent) const
{
    return !parent.isValid() ? m_screenshots.count() : 0;
}

QUrl ScreenshotsModel::screenshotAt(int row) const
{
    return m_screenshots[row];
}

int ScreenshotsModel::count() const
{
    return m_screenshots.count();
}
