/*
 *   Copyright (C) 2014 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library/Lesser General Public License
 *   version 2, or (at your option) any later version, as published by the
 *   Free Software Foundation
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details
 *
 *   You should have received a copy of the GNU Library/Lesser General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "PaginateModel.h"
#include <QDebug>

PaginateModel::PaginateModel(QObject* object)
    : QAbstractListModel(object)
    , m_firstItem(0)
    , m_pageSize(10)
    , m_sourceModel(0)
{
}

int PaginateModel::firstItem() const
{
    return m_firstItem;
}

void PaginateModel::setFirstItem(int row)
{
    Q_ASSERT(row>=0 && row<m_sourceModel->rowCount());
    if (row!=m_firstItem) {
        beginResetModel();
        m_firstItem = row;
        endResetModel();
        emit firstItemChanged();
    }
}

int PaginateModel::pageSize() const
{
    return m_pageSize;
}

void PaginateModel::setPageSize(int count)
{
    if (count != m_pageSize) {
        int difference = count - m_pageSize;
        if(difference>0) {
            beginInsertRows(QModelIndex(), m_pageSize, m_pageSize+difference);
            m_pageSize = count;
            endInsertRows();
        } else {
            beginRemoveRows(QModelIndex(), m_pageSize, m_pageSize+difference);
            m_pageSize = count;
            endRemoveRows();
        }
        emit pageSizeChanged();
    }
}

QAbstractItemModel* PaginateModel::sourceModel() const
{
    return m_sourceModel;
}

void PaginateModel::setSourceModel(QAbstractItemModel* model)
{
    if(m_sourceModel) {
        disconnect(m_sourceModel, 0, this, 0);
    }

    if(model!=m_sourceModel) {
        beginResetModel();
        m_sourceModel = model;
        if(model) {
            setRoleNames(model->roleNames());
            connect(m_sourceModel, SIGNAL(rowsAboutToBeInserted(QModelIndex,int,int)), SLOT(_k_sourceRowsAboutToBeInserted(QModelIndex,int,int)));
            connect(m_sourceModel, SIGNAL(rowsInserted(QModelIndex,int,int)), SLOT(_k_sourceRowsInserted(QModelIndex,int,int)));
            connect(m_sourceModel, SIGNAL(rowsAboutToBeRemoved(QModelIndex,int,int)), SLOT(_k_sourceRowsAboutToBeRemoved(QModelIndex,int,int)));
            connect(m_sourceModel, SIGNAL(rowsRemoved(QModelIndex,int,int)), SLOT(_k_sourceRowsRemoved(QModelIndex,int,int)));
            connect(m_sourceModel, SIGNAL(rowsAboutToBeMoved(QModelIndex,int,int,QModelIndex,int)), SLOT(_k_sourceRowsAboutToBeMoved(QModelIndex,int,int,QModelIndex,int)));
            connect(m_sourceModel, SIGNAL(rowsMoved(QModelIndex,int,int,QModelIndex,int)), SLOT(_k_sourceRowsMoved(QModelIndex,int,int,QModelIndex,int)));

            connect(m_sourceModel, SIGNAL(columnsAboutToBeInserted(QModelIndex,int,int)), SLOT(_k_sourceColumnsAboutToBeInserted(QModelIndex,int,int)));
            connect(m_sourceModel, SIGNAL(columnsInserted(QModelIndex,int,int)), SLOT(_k_sourceColumnsInserted(QModelIndex,int,int)));
            connect(m_sourceModel, SIGNAL(columnsAboutToBeRemoved(QModelIndex,int,int)), SLOT(_k_sourceColumnsAboutToBeRemoved(QModelIndex,int,int)));
            connect(m_sourceModel, SIGNAL(columnsRemoved(QModelIndex,int,int)), SLOT(_k_sourceColumnsRemoved(QModelIndex,int,int)));
            connect(m_sourceModel, SIGNAL(columnsAboutToBeMoved(QModelIndex,int,int,QModelIndex,int)), SLOT(_k_sourceColumnsAboutToBeMoved(QModelIndex,int,int,QModelIndex,int)));
            connect(m_sourceModel, SIGNAL(columnsMoved(QModelIndex,int,int,QModelIndex,int)), SLOT(_k_sourceColumnsMoved(QModelIndex,int,int,QModelIndex,int)));

            connect(m_sourceModel, SIGNAL(dataChanged(QModelIndex,QModelIndex)), SLOT(_k_sourceDataChanged(QModelIndex,QModelIndex)));
            connect(m_sourceModel, SIGNAL(headerDataChanged(Qt::Orientation,int,int)), SLOT(_k_sourceHeaderDataChanged(Qt::Orientation,int,int)));

            connect(m_sourceModel, SIGNAL(modelAboutToBeReset()), SLOT(_k_sourceModelAboutToBeReset()));
            connect(m_sourceModel, SIGNAL(modelReset()), SLOT(_k_sourceModelReset()));

            connect(m_sourceModel, SIGNAL(rowsInserted(QModelIndex,int,int)), SIGNAL(pageCountChanged()));
            connect(m_sourceModel, SIGNAL(rowsRemoved(QModelIndex,int,int)), SIGNAL(pageCountChanged()));
            connect(m_sourceModel, SIGNAL(modelReset()), SIGNAL(pageCountChanged()));
        }
        endResetModel();
        emit sourceModelChanged();
    }
}

int PaginateModel::rowCount(const QModelIndex& parent) const
{
    int ret = parent.isValid() || !m_sourceModel ? 0 : qMin(m_sourceModel->rowCount()-m_firstItem, m_pageSize);
    return ret;
}

QModelIndex PaginateModel::mapToSource(const QModelIndex& idx) const
{
    if(!m_sourceModel)
        return QModelIndex();
    return m_sourceModel->index(idx.row()+m_firstItem, idx.column());
}

QModelIndex PaginateModel::mapFromSource(const QModelIndex& idx) const
{
    Q_ASSERT(idx.model() == m_sourceModel);
    if(!m_sourceModel)
        return QModelIndex();
    return index(idx.row()-m_firstItem, idx.column());
}

QVariant PaginateModel::data(const QModelIndex& index, int role) const
{
    if(!m_sourceModel)
        return QVariant();
    QModelIndex idx = mapToSource(index);
    return idx.data(role);
}

void PaginateModel::firstPage()
{
    setFirstItem(0);
}

void PaginateModel::lastPage()
{
    setFirstItem(pageCount()*m_pageSize);
}

void PaginateModel::nextPage()
{
    setFirstItem(m_firstItem + m_pageSize);
}

void PaginateModel::previousPage()
{
    setFirstItem(m_firstItem - m_pageSize);
}

int PaginateModel::currentPage() const
{
    return m_firstItem/m_pageSize;
}

int PaginateModel::pageCount() const
{
    if(!m_sourceModel)
        return 0;
    int r = (m_sourceModel->rowCount()%m_pageSize == 0) ? 1 : 0;
    return m_sourceModel->rowCount()/m_pageSize - r;
}

//////////////////////////////

void PaginateModel::_k_sourceColumnsAboutToBeInserted(const QModelIndex& parent, int start, int end)
{
    Q_UNUSED(end)
    if(parent.isValid() || start!=0) {
        return;
    }
    beginResetModel();
}

void PaginateModel::_k_sourceColumnsAboutToBeMoved(const QModelIndex& sourceParent, int sourceStart, int sourceEnd, const QModelIndex& destParent, int dest)
{
    Q_UNUSED(sourceParent)
    Q_UNUSED(sourceStart)
    Q_UNUSED(sourceEnd)
    Q_UNUSED(destParent)
    Q_UNUSED(dest)
    beginResetModel();
}

void PaginateModel::_k_sourceColumnsAboutToBeRemoved(const QModelIndex& parent, int start, int end)
{
    Q_UNUSED(end)
    if(parent.isValid() || start!=0) {
        return;
    }
    beginResetModel();
}

void PaginateModel::_k_sourceColumnsInserted(const QModelIndex& parent, int start, int end)
{
    Q_UNUSED(end)
    if(parent.isValid() || start!=0) {
        return;
    }
    endResetModel();
}

void PaginateModel::_k_sourceColumnsMoved(const QModelIndex& sourceParent, int sourceStart, int sourceEnd, const QModelIndex& destParent, int dest)
{
    Q_UNUSED(sourceParent)
    Q_UNUSED(sourceStart)
    Q_UNUSED(sourceEnd)
    Q_UNUSED(destParent)
    Q_UNUSED(dest)
    endResetModel();
}

void PaginateModel::_k_sourceColumnsRemoved(const QModelIndex& parent, int start, int end)
{
    Q_UNUSED(end)
    if(parent.isValid() || start!=0) {
        return;
    }
    endResetModel();
}

void PaginateModel::_k_sourceDataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight)
{
    if(topLeft.parent().isValid() || bottomRight.row()<m_firstItem || topLeft.row()>m_firstItem+rowCount()) {
        return;
    }

    QModelIndex idxTop = mapFromSource(topLeft);
    QModelIndex idxBottom = mapFromSource(bottomRight);
    if(!idxTop.isValid())
        idxTop = index(0);
    if(!idxBottom.isValid())
        idxBottom = index(rowCount()-1);

    emit dataChanged(idxTop, idxBottom);
}

void PaginateModel::_k_sourceHeaderDataChanged(Qt::Orientation orientation, int first, int last)
{
    Q_UNUSED(last)
    if(first==0)
        emit headerDataChanged(orientation, 0, 0);
}

void PaginateModel::_k_sourceModelAboutToBeReset()
{
    beginResetModel();
}

void PaginateModel::_k_sourceModelReset()
{
    endResetModel();
}

void PaginateModel::_k_sourceRowsAboutToBeInserted(const QModelIndex& parent, int start, int end)
{
    if(!parent.isValid() || start>m_firstItem+m_pageSize) {
        return;
    }

    int insertedCount = end-start;
    if(insertedCount > m_pageSize) {
        beginResetModel();
    } else {
        int newStart = qMax(start-m_firstItem, 0);
        beginInsertRows(QModelIndex(), newStart, newStart+insertedCount);
    }
}

void PaginateModel::_k_sourceRowsAboutToBeMoved(const QModelIndex& sourceParent, int sourceStart, int sourceEnd, const QModelIndex& destParent, int dest)
{
    Q_UNUSED(sourceParent)
    Q_UNUSED(sourceStart)
    Q_UNUSED(sourceEnd)
    Q_UNUSED(destParent)
    Q_UNUSED(dest)
    //TODO could optimize, unsure if it makes sense
    beginResetModel();
}

void PaginateModel::_k_sourceRowsAboutToBeRemoved(const QModelIndex& parent, int start, int end)
{
    if(!parent.isValid() || start>m_firstItem+m_pageSize) {
        return;
    }

    int removedCount = end-start;
    if(removedCount > m_pageSize) {
        beginResetModel();
    } else {
        int newStart = qMax(start-m_firstItem, 0);
        beginRemoveRows(QModelIndex(), newStart, newStart+removedCount);
    }
}

void PaginateModel::_k_sourceRowsInserted(const QModelIndex& parent, int start, int end)
{
    if(!parent.isValid() || start>m_firstItem+m_pageSize) {
        return;
    }

    int insertedCount = end-start;
    if(insertedCount > m_pageSize) {
        endResetModel();
    } else {
        endInsertRows();
    }
}

void PaginateModel::_k_sourceRowsMoved(const QModelIndex& sourceParent, int sourceStart, int sourceEnd, const QModelIndex& destParent, int dest)
{
    Q_UNUSED(sourceParent)
    Q_UNUSED(sourceStart)
    Q_UNUSED(sourceEnd)
    Q_UNUSED(destParent)
    Q_UNUSED(dest)
    endResetModel();
}

void PaginateModel::_k_sourceRowsRemoved(const QModelIndex& parent, int start, int end)
{
    if(!parent.isValid() || start>m_firstItem+m_pageSize) {
        return;
    }

    int removedCount = end-start;
    if(removedCount > m_pageSize) {
        beginResetModel();
    } else {
        endRemoveRows();
    }
}
