/*
 *   SPDX-FileCopyrightText: 2012 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "ScreenshotsModel.h"
#include "libdiscover_debug.h"
#include <resources/AbstractResource.h>
// #include <QAbstractItemModelTester>

ScreenshotsModel::ScreenshotsModel(QObject *parent)
    : QAbstractListModel(parent)
    , m_resource(nullptr)
{
}

QHash<int, QByteArray> ScreenshotsModel::roleNames() const
{
    QHash<int, QByteArray> roles = QAbstractItemModel::roleNames();
    roles.insert(ThumbnailUrl, "small_image_url");
    roles.insert(ScreenshotUrl, "large_image_url");
    return roles;
}

void ScreenshotsModel::setResource(AbstractResource *res)
{
    if (res == m_resource)
        return;

    if (m_resource) {
        disconnect(m_resource, &AbstractResource::screenshotsFetched, this, &ScreenshotsModel::screenshotsFetched);
    }
    m_resource = res;
    Q_EMIT resourceChanged(res);

    if (res) {
        connect(m_resource, &AbstractResource::screenshotsFetched, this, &ScreenshotsModel::screenshotsFetched);
        res->fetchScreenshots();
    } else
        qCWarning(LIBDISCOVER_LOG) << "empty resource!";
}

AbstractResource *ScreenshotsModel::resource() const
{
    return m_resource;
}

void ScreenshotsModel::screenshotsFetched(const QList<QUrl> &thumbnails, const QList<QUrl> &screenshots)
{
    Q_ASSERT(thumbnails.count() == screenshots.count());
    if (thumbnails.isEmpty())
        return;

    beginInsertRows(QModelIndex(), m_thumbnails.size(), m_thumbnails.size() + thumbnails.size() - 1);
    m_thumbnails += thumbnails;
    m_screenshots += screenshots;
    endInsertRows();
    emit countChanged();
}

QVariant ScreenshotsModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.parent().isValid())
        return QVariant();

    switch (role) {
    case ThumbnailUrl:
        return m_thumbnails[index.row()];
    case ScreenshotUrl:
        return m_screenshots[index.row()];
    }

    return QVariant();
}

int ScreenshotsModel::rowCount(const QModelIndex &parent) const
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

void ScreenshotsModel::remove(const QUrl &url)
{
    int idxRemove = m_thumbnails.indexOf(url);
    if (idxRemove >= 0) {
        beginRemoveRows({}, idxRemove, idxRemove);
        m_thumbnails.removeAt(idxRemove);
        m_screenshots.removeAt(idxRemove);
        endRemoveRows();
        emit countChanged();

        qDebug() << "screenshot removed" << url;
    }
}
