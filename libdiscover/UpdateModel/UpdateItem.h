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

#ifndef UPDATEITEM_H
#define UPDATEITEM_H

// Qt includes
#include <QtCore/QList>
#include <QtCore/QString>
#include "discovercommon_export.h"

#include <QIcon>

class AbstractResource;
class DISCOVERCOMMON_EXPORT UpdateItem
{
public:
    enum class ItemType : quint8 {
        InvalidItem = 0,
        RootItem,
        CategoryItem,
        ApplicationItem
    };

    UpdateItem();
    UpdateItem(QString categoryName,
               QIcon categoryIcon);
    explicit UpdateItem(AbstractResource *app, UpdateItem *parent = nullptr);

    ~UpdateItem();

    UpdateItem *parent() const;
    void setParent(UpdateItem *parent);

    void appendChild(UpdateItem *child);
    bool removeChildren(int position, int count);
    QList<UpdateItem *> children() const;
    UpdateItem *child(int row) const;
    int childCount() const;
    int row() const;
    void sort();
    bool isEmpty() const;

    void setProgress(qreal progress);
    qreal progress() const;

    AbstractResource *app() const;
    QString name() const;
    QString version() const;
    QIcon icon() const;
    qint64 size() const;
    Qt::CheckState checked() const;
    ItemType type() const;

    int checkedItems() const;
    AbstractResource* resource() const { return m_app; }

private:
    AbstractResource *m_app;

    UpdateItem *m_parent;
    ItemType m_type;
    QList<UpdateItem *> m_children;
    QString m_categoryName;
    QIcon m_categoryIcon;
    qreal m_progress;
};

#endif // UPDATEITEM_H
