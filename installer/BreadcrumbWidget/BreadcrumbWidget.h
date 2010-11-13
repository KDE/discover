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

#ifndef BREADCRUMBWIDGET_H
#define BREADCRUMBWIDGET_H

#include <QtCore/QList>

#include <KHBox>

class BreadcrumbItem;

class BreadcrumbWidget : public KHBox
{
    Q_OBJECT
public:
    BreadcrumbWidget(QWidget *parent);
    ~BreadcrumbWidget();

    void setRootItem(BreadcrumbItem *root);
    void addLevel(BreadcrumbItem *crumb);

   /**
    * Removes the given item and all children
    *
    * @param The \c BreadcrumbItem to remove
    */
    void removeItem(BreadcrumbItem *crumb);

private:
    QList<BreadcrumbItem *> m_items;

    KHBox *m_breadcrumbArea;

private Q_SLOTS:
    void clearCrumbs();

Q_SIGNALS:
    void itemActivated(BreadcrumbItem *item);
};

#endif
