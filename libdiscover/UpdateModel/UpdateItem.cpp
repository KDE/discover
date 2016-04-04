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
#include <QDebug>

UpdateItem::UpdateItem()
    : m_app(nullptr)
    , m_parent(nullptr)
    , m_type(ItemType::RootItem)
    , m_progress(0)
{
}

UpdateItem::UpdateItem(QString categoryName,
                       QIcon categoryIcon)
    : m_app(nullptr)
    , m_parent(nullptr)
    , m_type(ItemType::CategoryItem)
    , m_categoryName(std::move(categoryName))
    , m_categoryIcon(std::move(categoryIcon))
    , m_progress(0)
{
}

UpdateItem::UpdateItem(AbstractResource *app, UpdateItem *parent)
    : m_app(app)
    , m_parent(parent)
    , m_type(ItemType::ApplicationItem)
    , m_progress(0)
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

void UpdateItem::setChildren(const QVector<UpdateItem*> &child)
{
    m_children = child;
    foreach(UpdateItem* item, m_children)
        item->setParent(this);
    sort();
}

QVector<UpdateItem *> UpdateItem::children() const
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
    case ItemType::RootItem:
        return {};
    }

    Q_UNREACHABLE();
    return QString();
}

QString UpdateItem::version() const
{
    switch (type()) {
    case ItemType::ApplicationItem:
        return m_app->availableVersion();
    case ItemType::CategoryItem:
    case ItemType::RootItem:
        return {};
    }

    Q_UNREACHABLE();
    return QString();
}

QIcon UpdateItem::icon() const
{
    switch (type()) {
    case ItemType::CategoryItem:
        return m_categoryIcon;
    case ItemType::ApplicationItem:
        return QIcon::fromTheme(m_app->icon());
    case ItemType::RootItem:
        return QIcon();
    }

    Q_UNREACHABLE();
    return QIcon();
}

qint64 UpdateItem::size() const
{
    ItemType itemType = type();
    qint64 size = 0;

    if (itemType == ItemType::ApplicationItem) {
        size = m_app->size();
    } else if (itemType == ItemType::CategoryItem) {
        foreach (UpdateItem *item, m_children) {
            size += item->app()->size();
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
        break;
    }

    return ret;
}

UpdateItem::ItemType UpdateItem::type() const
{
    return m_type;
}

int UpdateItem::checkedItems() const
{
    if (m_app)
        return checked()!=Qt::Unchecked ? 1 : 0;
    else {
        int ret = 0;
        foreach(UpdateItem* item, children()) {
            ret += item->checkedItems();
        }
        return ret;
    }
}

qreal UpdateItem::progress() const
{
    return m_progress;
}

void UpdateItem::setProgress(qreal progress)
{
    m_progress = progress;
}

QString UpdateItem::changelog() const
{
    return m_changelog;
}

void UpdateItem::setChangelog(const QString& changelog)
{
    m_changelog = changelog;
}
