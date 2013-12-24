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

#ifndef ABSTRACTVIEWBASE_H
#define ABSTRACTVIEWBASE_H

#include <QtWidgets/QWidget>

class QVBoxLayout;

class BreadcrumbItem;

class AbstractViewBase : public QWidget
{
    Q_OBJECT
public:
    AbstractViewBase(QWidget *parent);

    BreadcrumbItem *breadcrumbItem();
    bool isSearchable();
    virtual void search(const QString &text);

protected:
    QVBoxLayout *m_layout;
    BreadcrumbItem *m_crumb;
    bool m_searchable;

Q_SIGNALS:
    void switchToSubView(AbstractViewBase *view);
    void registerNewSubView(AbstractViewBase *view);
};

#endif
