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

#include <QtCore/QStringBuilder>

#include <Application.h>

UpdateItem::UpdateItem()
    : m_app(0)
    , m_parent(0)
{
}

UpdateItem::UpdateItem(const QString &categoryName,
                       const KIcon &categoryIcon)
    : m_app(0)
    , m_parent(0)
    , m_categoryName(categoryName)
    , m_categoryIcon(categoryIcon)
{
}

UpdateItem::UpdateItem(Application *app, UpdateItem *parent)
    : m_app(app)
    , m_parent(parent)
{
}

UpdateItem::~UpdateItem()
{
    delete m_app;
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

Application *UpdateItem::app() const
{
    return m_app;
}

QString UpdateItem::name() const
{
    switch (type()) {
    case CategoryItem:
        return m_categoryName;
    case ApplicationItem:
        if (m_app->package()->isForeignArch()) {
            return QString(m_app->name() % QLatin1String(" (")
                    % m_app->package()->architecture() % ')');
        }
        return m_app->name();
    default:
        break;
    }

    return QString();
}

KIcon UpdateItem::icon() const
{
    switch (type()) {
    case CategoryItem:
        return m_categoryIcon;
    case ApplicationItem:
        return KIcon(m_app->icon());
    default:
        return KIcon();
    }

    return KIcon();
}

qint64 UpdateItem::size() const
{
    int itemType = type();
    int size = 0;

    if (itemType == ApplicationItem) {
        size = m_app->package()->downloadSize();
    } else if (itemType == CategoryItem) {
        foreach (UpdateItem *item, m_children) {
            if (item->app()->package()->state() & QApt::Package::ToUpgrade) {
                size += item->size();
            }
        }
    }

    return size;
}

Qt::CheckState UpdateItem::checked() const
{
    int itemType = type();
    Qt::CheckState checkState = Qt::Unchecked;

    switch (itemType) {
    case CategoryItem: {
        int checkedCount = 0;
        int uncheckedCount = 0;

        foreach (UpdateItem *child, m_children) {
            (child->app()->package()->state() & QApt::Package::ToUpgrade) ?
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
    case ApplicationItem:
        (app()->package()->state() & QApt::Package::ToUpgrade) ?
                    checkState = Qt::Checked : checkState = Qt::Unchecked;
        break;
    }

    return checkState;
}

UpdateItem::ItemType UpdateItem::type() const
{
    if (!m_parent) {
        return RootItem;
    }

    // We now know we have a parent
    if (parent()->parent() == 0) {
        return CategoryItem;
    } else {
        return ApplicationItem;
    }
}
