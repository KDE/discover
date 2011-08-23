#include "UpdateModel.h"

// Qt includes
#include <QtGui/QFont>

// KDE includes
#include <KGlobal>
#include <KIconLoader>
#include <KLocale>
#include <KDebug>

// Own includes
#include "UpdateItem.h"
#include "../../installer/Application.h"

#define ICON_SIZE KIconLoader::SizeSmallMedium


UpdateModel::UpdateModel(QObject *parent) :
    QAbstractItemModel(parent)
{
    m_rootItem = new UpdateItem();
}

UpdateModel::~UpdateModel()
{
    delete m_rootItem;
}

QVariant UpdateModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) {
        return QVariant();
    }

    UpdateItem *item = static_cast<UpdateItem*>(index.internalPointer());
    int column = index.column();

    switch (role) {
    case Qt::DisplayRole:
        switch (column) {
        case 0:
            return item->name();
        case 1:
            return KGlobal::locale()->formatByteSize(item->size());
        }
            return QVariant();
        return QVariant();
    case Qt::DecorationRole:
        if (column == 0) {
            return item->icon().pixmap(ICON_SIZE, ICON_SIZE);
        }
    case Qt::FontRole: {
        QFont font;
        if ((item->type() == UpdateItem::CategoryItem) && column == 1) {
            font.setBold(true);
            return font;
        }
        return font;
    }
    case Qt::CheckStateRole:
        if (column == 0) {
            return item->checked();
        }
        return QVariant();
    default:
        return QVariant();
    }

    return QVariant();
}

QVariant UpdateModel::headerData(int section, Qt::Orientation orientation,
                                int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
    {
        switch (section) {
        case 0:
            return i18nc("@label Column label", "Updates");
        case 1:
            return i18nc("@label Column label", "Download Size");
        }
    }

    return QVariant();
}

Qt::ItemFlags UpdateModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return 0;

    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

QModelIndex UpdateModel::index(int row, int column, const QModelIndex &index) const
{
    if (index.isValid() && (index.column() != 0 && index.column() != 1)) {
        return QModelIndex();
    }

    if (UpdateItem *parent = itemFromIndex(index)) {
        if (UpdateItem *childItem = parent->child(row)) {
            return createIndex(row, column, childItem);
        }
    }

    return QModelIndex();
}

QModelIndex UpdateModel::parent(const QModelIndex &index) const
{
    if (!index.isValid()) {
        return QModelIndex();
    }

    UpdateItem *childItem = static_cast<UpdateItem*>(index.internalPointer());
    UpdateItem *parentItem = childItem->parent();

    if (parentItem == m_rootItem) {
        return QModelIndex();
    }

    return createIndex(parentItem->row(), 0, parentItem);
}

int UpdateModel::rowCount(const QModelIndex &parent) const
{
    UpdateItem *parentItem = itemFromIndex(parent);

    return parentItem->childCount();
}

int UpdateModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return 2;
}

UpdateItem* UpdateModel::itemFromIndex(const QModelIndex &index) const
{
    if (index.isValid())
         return static_cast<UpdateItem*>(index.internalPointer());
     return m_rootItem;
}

void UpdateModel::addItem(UpdateItem *item)
{
    beginInsertRows(QModelIndex(), m_rootItem->childCount(), m_rootItem->childCount());
    m_rootItem->appendChild(item);
    endInsertRows();
}

bool UpdateModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (role == Qt::CheckStateRole) {
        UpdateItem *item = static_cast<UpdateItem*>(index.internalPointer());
        bool newValue = value.toBool();
        int type = item->type();

        if (type == UpdateItem::CategoryItem) {
            QList<Application *> apps;

            // Collect items to (un)check
            foreach (UpdateItem *child, item->children()) {
                apps << child->app();
            }

            emit checkApps(apps, newValue);
        } else if (type == UpdateItem::ApplicationItem) {
            emit checkApp(item->app(), newValue);
        }
        return true;
    }

    return false;
}

void UpdateModel::packageChanged()
{
    // We don't know what changed or not, so say everything changed
    emit dataChanged(index(0, 0), QModelIndex());
}
