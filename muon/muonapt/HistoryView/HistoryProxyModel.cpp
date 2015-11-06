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

#include "HistoryProxyModel.h"

#include <QStandardItemModel>
#include <QStandardItem>

HistoryProxyModel::HistoryProxyModel(QObject *parent)
    : QSortFilterProxyModel(parent)
    , m_stateFilter((QApt::Package::State)0)
{
}

HistoryProxyModel::~HistoryProxyModel()
{
}

void HistoryProxyModel::search(const QString &searchText)
{
    m_searchText = searchText;
    invalidate();
}

void HistoryProxyModel::setStateFilter(QApt::Package::State state)
{
    m_stateFilter = state;
    invalidate();
}

bool HistoryProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    QModelIndex sourceIndex = sourceModel()->index(sourceRow, 0, sourceParent);
        for(int i = 0 ; i < sourceModel()->rowCount(sourceIndex); i++) {
            if (filterAcceptsRow(i, sourceIndex)) {
                return true;
            }
        }

    //Our "main"-method
    QStandardItem *item = static_cast<QStandardItemModel *>(sourceModel())->itemFromIndex(sourceModel()->index(sourceRow, 0, sourceParent));

    if (!item) {
        return false;
    }

    if (!m_stateFilter == 0) {
        if ((bool)(item->data(HistoryActionRole).toInt() & m_stateFilter) == false) {
            return false;
        }
    }

    if (!m_searchText.isEmpty()) {
        if ((bool)(item->data(Qt::DisplayRole).toString().contains(m_searchText)) == false) {
            return false;
        }
    }

    return true;
}

bool HistoryProxyModel::lessThan(const QModelIndex &left, const QModelIndex &right) const
{
    QStandardItemModel *parentModel = static_cast<QStandardItemModel *>(sourceModel());

    QStandardItem *leftItem = parentModel->itemFromIndex(left);
    QStandardItem *rightItem = parentModel->itemFromIndex(right);

    return (leftItem->data(HistoryDateRole).toDateTime() > rightItem->data(HistoryDateRole).toDateTime());
}
