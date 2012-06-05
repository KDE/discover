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

#ifndef RESOURCESMODEL_H
#define RESOURCESMODEL_H

#include <QtCore/QModelIndex>
#include <QVector>

#include "libmuonprivate_export.h"

class AbstractResource;
class AbstractResourcesBackend;

class MUONPRIVATE_EXPORT ResourcesModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int updatesCount READ updatesCount NOTIFY updatesCountChanged)
    public:
        enum Roles {
            NameRole = Qt::UserRole,
            IconRole,
            CommentRole,
            ActionRole,
            StatusRole,
            RatingRole,
            RatingPointsRole,
            SortableRatingRole,
            ActiveRole,
            ProgressRole,
            ProgressTextRole,
            InstalledRole,
            ApplicationRole,
            UsageCountRole,
            PopConRole,
            UntranslatedNameRole,
            OriginRole,
            CanUpgrade,
            PackageNameRole,
            IsTechnicalRole,
            CategoryRole,
            SectionRole
        };
        explicit ResourcesModel(QObject* parent=0);
        
        virtual QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;
        virtual int rowCount(const QModelIndex& parent = QModelIndex()) const;
        
        void addResourcesBackend(AbstractResourcesBackend* resources);
        
        AbstractResource* applicationByPackageName(const QString& name);
        AbstractResource* resourceAt(int row) const;
        QVector< AbstractResourcesBackend* > backends() const;
        Q_SCRIPTABLE AbstractResourcesBackend* backendForResource(AbstractResource* resource) const;
        int updatesCount() const;

    signals:
        void updatesCountChanged();

    private slots:
        void cleanCaller();
        void resetCaller();
        
    private:
        QVector< AbstractResourcesBackend* > m_backends;
        QVector< QVector<AbstractResource*> > m_resources;
};

#endif // RESOURCESMODEL_H
