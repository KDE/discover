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

#include <QtCore/QStringBuilder>
#include <KLocalizedString>
#include <LibQApt/Package>
#include <LibQApt/Backend>

UpdateItem::UpdateItem()
    : m_app(0)
    , m_parent(0)
    , m_type(ItemType::RootItem)
    , m_checkState(Qt::Unchecked)
{
}

UpdateItem::UpdateItem(const QString &categoryName,
                       const KIcon &categoryIcon)
    : m_app(0)
    , m_parent(0)
    , m_type(ItemType::CategoryItem)
    , m_checkState(Qt::Unchecked)
    , m_categoryName(categoryName)
    , m_categoryIcon(categoryIcon)
{
}

UpdateItem::UpdateItem(AbstractResource *app, UpdateItem *parent)
    : m_app(app)
    , m_parent(parent)
    , m_type(ItemType::ApplicationItem)
    , m_checkState(Qt::Unchecked)
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
    child->setParent(this);
    m_children.append(child);
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
    case ItemType::ApplicationItem: {
        QApt::Package* p = retrievePackage();
        if (p->isForeignArch()) {
            return i18n("%1 (%2)", m_app->name(), p->architecture());
        }
    }   return m_app->name();
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

KIcon UpdateItem::icon() const
{
    switch (type()) {
    case ItemType::CategoryItem:
        return m_categoryIcon;
    case ItemType::ApplicationItem:
        return KIcon(m_app->icon());
    default:
        return KIcon();
    }

    return KIcon();
}

qint64 UpdateItem::size() const
{
    ItemType itemType = type();
    int size = 0;

    if (itemType == ItemType::ApplicationItem) {
        size = retrievePackage()->downloadSize();
    } else if (itemType == ItemType::CategoryItem) {
        foreach (UpdateItem *item, m_children) {
            if (item->app()->state() & AbstractResource::Upgradeable) {
                size += item->size();
            }
        }
    }

    return size;
}

Qt::CheckState UpdateItem::checked() const
{
    ItemType itemType = type();
    Qt::CheckState checkState = Qt::Unchecked;

    switch (itemType) {
    case ItemType::CategoryItem: {
        int checkedCount = 0;
        int uncheckedCount = 0;

        foreach (UpdateItem *child, m_children) {
            (child->checked() == Qt::Checked) ?
                        checkedCount++ : uncheckedCount++;
        }

        if (checkedCount && uncheckedCount) {
            checkState = Qt::PartiallyChecked;
            break;
        }

        if ((checkedCount && !uncheckedCount)) {
            checkState = Qt::Checked;
        } else {
            checkState = Qt::Unchecked;
        }
        break;
    }
    case ItemType::ApplicationItem:
        return m_checkState;
        break;
    case ItemType::RootItem:
    case ItemType::InvalidItem:
        break;
    }

    return checkState;
}

void UpdateItem::setChecked(bool checked)
{
    if (type() == ItemType::ApplicationItem || type() == ItemType::CategoryItem)
        m_checkState = (checked) ? Qt::Checked : Qt::Unchecked;
}

UpdateItem::ItemType UpdateItem::type() const
{
    return m_type;
}

QApt::Package* UpdateItem::retrievePackage() const
{
    QApt::Backend* backend = qobject_cast<QApt::Backend*>(m_app->backend()->property("backend").value<QObject*>());
    return backend->package(m_app->packageName());
}
