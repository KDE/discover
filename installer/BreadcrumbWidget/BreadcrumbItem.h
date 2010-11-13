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

#ifndef BREADCRUMBITEM_H
#define BREADCRUMBITEM_H

#include <KHBox>

class QIcon;

class AbstractViewBase;
class BreadcrumbItemButton;

class BreadcrumbItem : public KHBox
{
    Q_OBJECT
public:
    BreadcrumbItem(QWidget *parent);
    ~BreadcrumbItem();

    BreadcrumbItem *childItem() const;
    AbstractViewBase *associatedView() const;
    bool hasChildren() const;

    void setChildItem(BreadcrumbItem *child);
    void setAssociatedView(AbstractViewBase *view);
    void setText(const QString &text);
    void setIcon(const QIcon &icon);
    void setActive(bool active);

private:
    BreadcrumbItem *m_childItem;

    bool m_hasChildren;
    AbstractViewBase *m_associatedView;
    BreadcrumbItemButton *m_button;

private Q_SLOTS:
    void emitActivated();

Q_SIGNALS:
    void activated(BreadcrumbItem *item);
};

#endif
