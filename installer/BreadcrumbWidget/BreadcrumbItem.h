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

class QPushButton;

// FIXME: Should be : public BreadcrumbButton, once it exists
class BreadcrumbItem : public KHBox
{
    Q_OBJECT
public:
    BreadcrumbItem(QWidget *parent);
    ~BreadcrumbItem();

    BreadcrumbItem *childItem() const;
    QWidget *associatedWidget() const;
    bool hasChildren() const;

    void setChildItem(BreadcrumbItem *child);
    void setAssociatedWidget(QWidget *widget);
    void setText(const QString &text);

private:
    BreadcrumbItem *m_childItem;

    bool m_hasChildren;
    QWidget *m_associatedWidget;
    QPushButton *m_button;

private Q_SLOTS:
    void emitActivated();

Q_SIGNALS:
    void activated(BreadcrumbItem *item);
};

#endif
