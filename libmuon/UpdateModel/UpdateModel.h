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

#ifndef UPDATEMODEL_H
#define UPDATEMODEL_H

#include <QtCore/QAbstractItemModel>
#include "libMuonCommon_export.h"

class ResourcesUpdatesModel;
class AbstractResource;
class UpdateItem;

class MUONCOMMON_EXPORT UpdateModel : public QAbstractItemModel
{
    Q_OBJECT
    Q_PROPERTY(ResourcesUpdatesModel* backend READ backend WRITE setBackend)
public:
    explicit UpdateModel(QObject *parent = nullptr);
    ~UpdateModel();

    QVariant data(const QModelIndex &index, int role) const override;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    QModelIndex index(int row, int column,
                      const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &index) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    bool setData(const QModelIndex &index, const QVariant &value, int role) override;
    void setResources(const QList<AbstractResource*>& res);
    UpdateItem *itemFromIndex(const QModelIndex &index) const;

    void checkResources(const QList< AbstractResource* >& resource, bool checked);
    QHash<int,QByteArray> roleNames() const override;

    enum Columns {
        NameColumn = 0,
        VersionColumn,
        SizeColumn
    };
    ResourcesUpdatesModel* backend() const;

public Q_SLOTS:
    void setBackend(ResourcesUpdatesModel* updates);

private:
    void activityChanged();

    void addResource(AbstractResource* res);
    UpdateItem *m_rootItem;
    ResourcesUpdatesModel* m_updates;
};

#endif // UPDATEMODEL_H
