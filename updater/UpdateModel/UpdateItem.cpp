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

#include "UpdateItem.h"
#include <resources/AbstractResource.h>
#include <resources/AbstractResourcesBackend.h>
#include <resources/AbstractBackendUpdater.h>

#include <QtCore/QStringBuilder>
#include <KLocalizedString>

UpdateItem::UpdateItem()
    : m_app(0)
    , m_parent(0)
    , m_type(ItemType::RootItem)
{
}

UpdateItem::UpdateItem(const QString &categoryName,
                       const QIcon &categoryIcon)
    : m_app(0)
    , m_parent(0)
    , m_type(ItemType::CategoryItem)
    , m_categoryName(categoryName)
    , m_categoryIcon(categoryIcon)
{
}

UpdateItem::UpdateItem(AbstractResource *app, UpdateItem *parent)
    : m_app(app)
    , m_parent(parent)
    , m_type(ItemType::ApplicationItem)
{
}

UpdateItem::~UpdateItem()
{
    qDeleteAll(m_children);
}

UpdateItem *UpdateItem::parent() const
{
    return m_parent;
}

void UpdateItem::setParent(UpdateItem *parent)
{
    m_parent = parent;
}

void UpdateItem::appendChild(UpdateItem *child)
{
    if(!m_children.contains(child)) {
        child->setParent(this);
        m_children.append(child);
    }
}

bool UpdateItem::removeChildren(int position, int count)
{
    if (position < 0 || position > m_children.size())
        return false;

    for (int row = 0; row < count; ++row)
        delete m_children.takeAt(position);

    return true;
}

QList<UpdateItem *> UpdateItem::children() const
{
    return m_children;
}

UpdateItem *UpdateItem::child(int row) const
{
    return m_children.value(row);
}

int UpdateItem::childCount() const
{
    return m_children.count();
}

bool UpdateItem::isEmpty() const
{
    return m_children.isEmpty();
}

int UpdateItem::row() const
{
    if (m_parent)
        return m_parent->m_children.indexOf(const_cast<UpdateItem*>(this));

    return 0;
}

void UpdateItem::sort()
{
    qSort(m_children.begin(), m_children.end(),
          [](UpdateItem *a, UpdateItem *b) { return a->name() < b->name(); });
}

AbstractResource *UpdateItem::app() const
{
    return m_app;
}

QString UpdateItem::name() const
{
    switch (type()) {
    case ItemType::CategoryItem:
        return m_categoryName;
    case ItemType::ApplicationItem:
        return m_app->name();
    default:
        break;
    }

    return QString();
}

QString UpdateItem::version() const
{
    switch (type()) {
    case ItemType::ApplicationItem:
        return m_app->availableVersion();
    case ItemType::CategoryItem:
    default:
        break;
    }

    return QString();
}

QIcon UpdateItem::icon() const
{
    switch (type()) {
    case ItemType::CategoryItem:
        return m_categoryIcon;
    case ItemType::ApplicationItem:
        return QIcon::fromTheme(m_app->icon());
    default:
        return QIcon();
    }

    return QIcon();
}

qint64 UpdateItem::size() const
{
    ItemType itemType = type();
    int size = 0;

    if (itemType == ItemType::ApplicationItem) {
        size = m_app->downloadSize();
    } else if (itemType == ItemType::CategoryItem) {
        foreach (UpdateItem *item, m_children) {
            size += item->app()->downloadSize();
        }
    }

    return size;
}

static bool isMarked(AbstractResource* res)
{
    return res->backend()->backendUpdater()->isMarked(res);
}

Qt::CheckState UpdateItem::checked() const
{
    Qt::CheckState ret = Qt::Unchecked;

    switch (type()) {
    case ItemType::CategoryItem: {
        int checkedCount = 0;
        foreach(UpdateItem* child, children()) {
            checkedCount += isMarked(child->app());
        }
        ret = checkedCount==0 ? Qt::Unchecked : 
              checkedCount==childCount() ? Qt::Checked : Qt::PartiallyChecked;
    }   break;
    case ItemType::ApplicationItem:
        Q_ASSERT(app());
        ret = isMarked(app()) ? Qt::Checked : Qt::Unchecked;
        break;
    case ItemType::RootItem:
    case ItemType::InvalidItem:
        break;
    }

    return ret;
}

UpdateItem::ItemType UpdateItem::type() const
{
    return m_type;
}
