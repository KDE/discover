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
#include <QtMath>
#include <QDebug>

PaginateModel::PaginateModel(QObject* object)
    : QAbstractListModel(object)
    , m_firstItem(0)
    , m_pageSize(10)
    , m_sourceModel(nullptr)
    , m_hasStaticRowCount(false)
{
}

PaginateModel::~PaginateModel()
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
        const int oldSize = rowsByPageSize(m_pageSize);
        const int newSize = rowsByPageSize(count);
        const int difference = newSize - oldSize;
        if (difference==0) {
            m_pageSize = count;
        } else if(difference>0) {
            beginInsertRows(QModelIndex(), m_pageSize, m_pageSize+difference-1);
            m_pageSize = count;
            endInsertRows();
        } else {
            beginRemoveRows(QModelIndex(), m_pageSize+difference, m_pageSize-1);
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
        disconnect(m_sourceModel, nullptr, this, nullptr);
    }

    if(model!=m_sourceModel) {
        beginResetModel();
        m_sourceModel = model;
        if(model) {
            connect(m_sourceModel, &QAbstractItemModel::rowsAboutToBeInserted, this, &PaginateModel::_k_sourceRowsAboutToBeInserted);
            connect(m_sourceModel, &QAbstractItemModel::rowsInserted, this, &PaginateModel::_k_sourceRowsInserted);
            connect(m_sourceModel, &QAbstractItemModel::rowsAboutToBeRemoved, this, &PaginateModel::_k_sourceRowsAboutToBeRemoved);
            connect(m_sourceModel, &QAbstractItemModel::rowsRemoved, this, &PaginateModel::_k_sourceRowsRemoved);
            connect(m_sourceModel, &QAbstractItemModel::rowsAboutToBeMoved, this, &PaginateModel::_k_sourceRowsAboutToBeMoved);
            connect(m_sourceModel, &QAbstractItemModel::rowsMoved, this, &PaginateModel::_k_sourceRowsMoved);

            connect(m_sourceModel, &QAbstractItemModel::columnsAboutToBeInserted, this, &PaginateModel::_k_sourceColumnsAboutToBeInserted);
            connect(m_sourceModel, &QAbstractItemModel::columnsInserted, this, &PaginateModel::_k_sourceColumnsInserted);
            connect(m_sourceModel, &QAbstractItemModel::columnsAboutToBeRemoved, this, &PaginateModel::_k_sourceColumnsAboutToBeRemoved);
            connect(m_sourceModel, &QAbstractItemModel::columnsRemoved, this, &PaginateModel::_k_sourceColumnsRemoved);
            connect(m_sourceModel, &QAbstractItemModel::columnsAboutToBeMoved, this, &PaginateModel::_k_sourceColumnsAboutToBeMoved);
            connect(m_sourceModel, &QAbstractItemModel::columnsMoved, this, &PaginateModel::_k_sourceColumnsMoved);

            connect(m_sourceModel, &QAbstractItemModel::dataChanged, this, &PaginateModel::_k_sourceDataChanged);
            connect(m_sourceModel, &QAbstractItemModel::headerDataChanged, this, &PaginateModel::_k_sourceHeaderDataChanged);

            connect(m_sourceModel, &QAbstractItemModel::modelAboutToBeReset, this, &PaginateModel::_k_sourceModelAboutToBeReset);
            connect(m_sourceModel, &QAbstractItemModel::modelReset, this, &PaginateModel::_k_sourceModelReset);

            connect(m_sourceModel, &QAbstractItemModel::rowsInserted, this, &PaginateModel::pageCountChanged);
            connect(m_sourceModel, &QAbstractItemModel::rowsRemoved, this, &PaginateModel::pageCountChanged);
            connect(m_sourceModel, &QAbstractItemModel::modelReset, this, &PaginateModel::pageCountChanged);
        }
        endResetModel();
        emit sourceModelChanged();
    }
}

QHash< int, QByteArray > PaginateModel::roleNames() const
{
    return m_sourceModel ? m_sourceModel->roleNames() : QAbstractItemModel::roleNames();
}

int PaginateModel::rowsByPageSize(int size) const
{
    return m_hasStaticRowCount ? size
        : !m_sourceModel ? 0
        : qMin(m_sourceModel->rowCount()-m_firstItem, size);
}

int PaginateModel::rowCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : rowsByPageSize(m_pageSize);
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
    setFirstItem((pageCount() - 1)*m_pageSize);
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
    const int r = (m_sourceModel->rowCount()%m_pageSize == 0) ? 1 : 0;
    return qCeil(float(m_sourceModel->rowCount())/m_pageSize) - r;
}

bool PaginateModel::hasStaticRowCount() const
{
    return m_hasStaticRowCount;
}

void PaginateModel::setStaticRowCount(bool src)
{
    if (src == m_hasStaticRowCount) {
        return;
    }

    m_hasStaticRowCount = src;
    beginResetModel();
    endResetModel();
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

void PaginateModel::_k_sourceDataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight, const QVector<int> &roles)
{
    if(topLeft.parent().isValid() || bottomRight.row()<m_firstItem || topLeft.row()>lastItem()) {
        return;
    }

    QModelIndex idxTop = mapFromSource(topLeft);
    QModelIndex idxBottom = mapFromSource(bottomRight);
    if(!idxTop.isValid())
        idxTop = index(0);
    if(!idxBottom.isValid())
        idxBottom = index(rowCount()-1);

    emit dataChanged(idxTop, idxBottom, roles);
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

bool PaginateModel::isIntervalValid(const QModelIndex& parent, int start, int /*end*/) const
{
    return !parent.isValid() && start<=lastItem();
}

bool PaginateModel::canSizeChange() const
{
    return !m_hasStaticRowCount && currentPage() == pageCount()-1;
}

void PaginateModel::_k_sourceRowsAboutToBeInserted(const QModelIndex& parent, int start, int end)
{
    if(!isIntervalValid(parent, start, end)) {
        return;
    }

    if(canSizeChange()) {
        const int insertedCount = end-start;
        const int newStart = qMax(start-m_firstItem, 0);
        beginInsertRows(QModelIndex(), newStart, newStart+insertedCount);
    } else {
        beginResetModel();
    }
}

void PaginateModel::_k_sourceRowsInserted(const QModelIndex& parent, int start, int end)
{
    if(!isIntervalValid(parent, start, end)) {
        return;
    }

    if(canSizeChange()) {
        endInsertRows();
    } else {
        endResetModel();
    }
}

void PaginateModel::_k_sourceRowsAboutToBeMoved(const QModelIndex& sourceParent, int sourceStart, int sourceEnd, const QModelIndex& destParent, int dest)
{
    Q_UNUSED(sourceParent)
    Q_UNUSED(sourceStart)
    Q_UNUSED(sourceEnd)
    Q_UNUSED(destParent)
    Q_UNUSED(dest)
    //NOTE could optimize, unsure if it makes sense
    beginResetModel();
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

void PaginateModel::_k_sourceRowsAboutToBeRemoved(const QModelIndex& parent, int start, int end)
{
    if(!isIntervalValid(parent, start, end)) {
        return;
    }

    if(canSizeChange()) {
        const int removedCount = end-start;
        const int newStart = qMax(start-m_firstItem, 0);
        beginRemoveRows(QModelIndex(), newStart, newStart+removedCount);
    } else {
        beginResetModel();
    }
}

void PaginateModel::_k_sourceRowsRemoved(const QModelIndex& parent, int start, int end)
{
    if(!isIntervalValid(parent, start, end)) {
        return;
    }

    if(canSizeChange()) {
        endRemoveRows();
    } else {
        beginResetModel();
    }
}

int PaginateModel::lastItem() const
{
    return m_firstItem + rowCount();
}
