/***************************************************************************
 *   Copyright © 2010 Guillaume Martres <smarter@ubuntu.com>               *
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

#ifndef DOWNLOADMODEL_H
#define DOWNLOADMODEL_H

#include <QList>
#include <QHash>
#include <QModelIndex>

//TODO: global.h for enum
#include "../PackageModel/PackageModel.h"

class DownloadModel : public QAbstractListModel
{
    Q_OBJECT
public:
    explicit DownloadModel(QObject *parent = 0);
    virtual QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;
    virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    virtual int columnCount(const QModelIndex& parent = QModelIndex()) const;
    virtual int rowCount(const QModelIndex& parent = QModelIndex()) const;

public Q_SLOTS:
    void updatePercentage(const QString &package, int percentage);

Q_SIGNALS:
    void dataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight);

private:
    QList< QPair<QString, int> > m_packageList;
};

#endif // DOWNLOADMODEL_H
