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

#ifndef AVAILABLEVIEW_H
#define AVAILABLEVIEW_H

#include <QtGui/QWidget>

class QStackedWidget;

class AbstractViewBase;
class ApplicationBackend;
class BreadcrumbItem;
class BreadcrumbWidget;
class Category;
class CategoryViewWidget;

namespace QApt {
    class Backend;
}

class AvailableView : public QWidget
{
    Q_OBJECT
public:
    AvailableView(QWidget *parent, ApplicationBackend *m_appBackend);
    ~AvailableView();

private:
    QApt::Backend *m_backend;
    ApplicationBackend *m_appBackend;
    QList<Category *> m_categoryList;

    QStackedWidget *m_viewStack;
    BreadcrumbWidget *m_breadcrumbWidget;
    CategoryViewWidget *m_categoryViewWidget;

public Q_SLOTS:
    void setBackend(QApt::Backend *backend);

private Q_SLOTS:
    void populateCategories();
    void registerNewSubView(AbstractViewBase *subView);
    void activateBreadcrumbItem(BreadcrumbItem *item);
    void switchToSubView(AbstractViewBase *subView);
};

#endif
