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

#ifndef PACKAGEPROXYMODEL_H
#define PACKAGEPROXYMODEL_H

#include <QSortFilterProxyModel>
#include <QString>

#include <libqapt/package.h>

namespace QApt {
    class Backend;
}

class PackageProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT
public:
    PackageProxyModel(QObject *parent);
    ~PackageProxyModel();

    void setBackend(QApt::Backend *backend);
    void search(const QString &searchText);
    void setGroupFilter(const QString &filterText);
    void setStateFilter(QApt::Package::State state);
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const;
    QApt::Package *packageAt(const QModelIndex &index) const;
    void reset();

protected:
    bool lessThan(const QModelIndex &left, const QModelIndex &right) const;

private:
    QApt::Backend *m_backend;
    QApt::PackageList m_packages;
    QString m_searchText;
    QString m_groupFilter;
    QApt::Package::State m_stateFilter;
    bool m_sortByRelevancy;
};

#endif
