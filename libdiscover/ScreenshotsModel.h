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

#ifndef SCREENSHOTSMODEL_H
#define SCREENSHOTSMODEL_H

#include <QModelIndex>
#include <QUrl>
#include "discovercommon_export.h"

class AbstractResource;

class DISCOVERCOMMON_EXPORT ScreenshotsModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(AbstractResource* application READ resource WRITE setResource)
    Q_PROPERTY(int count READ count NOTIFY countChanged)
    public:
        enum Roles { ThumbnailUrl=Qt::UserRole+1, ScreenshotUrl };
        
        ScreenshotsModel(QObject* parent = nullptr);
        QHash<int, QByteArray> roleNames() const override;

        AbstractResource* resource() const;
        void setResource(AbstractResource* res);

        QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
        int rowCount(const QModelIndex& parent = QModelIndex()) const override;
        Q_SCRIPTABLE QUrl screenshotAt(int row) const;
        int count() const;

    private Q_SLOTS:
        void screenshotsFetched(const QList<QUrl>& thumbnails, const QList<QUrl>& screenshots);

    Q_SIGNALS:
        void countChanged();

    private:
        AbstractResource* m_resource;
        QList<QUrl> m_thumbnails;
        QList<QUrl> m_screenshots;

};

#endif // SCREENSHOTSMODEL_H
