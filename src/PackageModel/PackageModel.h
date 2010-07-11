/***************************************************************************
 *   Copyright © 2010 Jonathan Thomas <echidnaman@kubuntu.org>             *
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

#ifndef PACKAGEMODEL_H
#define PACKAGEMODEL_H

#include <QAbstractListModel>

#include <libqapt/package.h>

class PackageModel: public QAbstractListModel
{
    Q_OBJECT
public:
    enum {
        NameRole = Qt::UserRole,
        IconRole = Qt::UserRole + 1,
        DescriptionRole = Qt::UserRole + 2,
        ActionRole = Qt::UserRole + 3,
        StatusRole = Qt::UserRole + 4
    };
    explicit PackageModel(QObject *parent = 0);
    ~PackageModel();

    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;

    void setPackages(const QApt::PackageList &list);
    void clear();
    QApt::Package *packageAt(const QModelIndex &index) const;
    QApt::PackageList packages() const;

private:
    QApt::PackageList m_packages;
};

#endif
