/***************************************************************************
 *   Copyright © 2016 Aleix Pol Gonzalez <aleixpol@blue-systems.com>       *
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

#ifndef FEATUREDMODEL_H
#define FEATUREDMODEL_H

#include <QAbstractListModel>
#include <QPointer>

namespace KIO { class StoredTransferJob; }
class AbstractResource;
class AbstractResourcesBackend;

class FeaturedModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(bool isFetching READ isFetching NOTIFY isFetchingChanged)
    public:
        FeaturedModel();
        ~FeaturedModel() override {}

        void setResources(const QVector<AbstractResource*>& resources);
        QVariant data(const QModelIndex & index, int role) const override;
        int rowCount(const QModelIndex & parent) const override;
        QHash<int, QByteArray> roleNames() const override;

        bool isFetching() const { return m_isFetching != 0; }

    Q_SIGNALS:
        void isFetchingChanged();

    private:
        void refreshCurrentApplicationBackend();
        void setUris(const QVector<QUrl> &uris);
        void refresh();
        void removeResource(AbstractResource* resource);

        void acquireFetching(bool f);

        QVector<AbstractResource*> m_resources;
        int m_isFetching = 0;
        AbstractResourcesBackend* m_backend = nullptr;
};

#endif // FEATUREDMODEL_H
