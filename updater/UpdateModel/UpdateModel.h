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

class AbstractResource;
class UpdateItem;

namespace QApt {
    class Package;
}

class UpdateModel : public QAbstractItemModel
{
    Q_OBJECT
public:
    explicit UpdateModel(QObject *parent = 0);
    ~UpdateModel();

    QVariant data(const QModelIndex &index, int role) const;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role) const;
    Qt::ItemFlags flags(const QModelIndex &index) const;
    QModelIndex index(int row, int column,
                      const QModelIndex &parent = QModelIndex()) const;
    QModelIndex parent(const QModelIndex &index) const;
    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;

    void clear();
    bool removeItem(const QModelIndex &index);
    bool removeRows(int position, int rows, const QModelIndex &index);
    QModelIndexList collectItems(const QModelIndex &parent) const;
    UpdateItem *itemFromIndex(const QModelIndex &index) const;

    bool setData(const QModelIndex &index, const QVariant &value, int role);
    void addResources(const QList<AbstractResource*>& res);

    enum Columns {
        NameColumn = 0,
        VersionColumn,
        SizeColumn
    };

private:
    void addResource(AbstractResource* res);
    void addItem(UpdateItem *item);
    UpdateItem *m_rootItem;
    UpdateItem* m_systemItem;
    UpdateItem* m_appItem;
    UpdateItem* m_securityItem;

public Q_SLOTS:
    void packageChanged();

Q_SIGNALS:
    void checkApps(const QList<AbstractResource*>& apps, bool checked);
};

#endif // UPDATEMODEL_H
