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

#include <QModelIndex>
#include <QtGui/QWidget>

class QStackedWidget;
class QStandardItemModel;

class Application;
class ApplicationBackend;
class ApplicationView;
class ApplicationWidget;
class BreadcrumbItem;
class BreadcrumbWidget;
class Category;
class CategoryView;

namespace QApt {
    class Backend;
}

enum CategoryModelRole {
    CategoryTypeRole = Qt::UserRole + 1,
    AndOrFilterRole = Qt::UserRole + 2,
    NotFilterRolr = Qt::UserRole + 3
};

enum CatViewType {
    /// An invalid type
    InvalidType = 0,
    /// An AppView since there are no sub-cats
    CategoryType = 1,
    /// A SubCategoryView
    SubCatType = 2
};

class AvailableView : public QWidget
{
    Q_OBJECT
public:
    AvailableView(QWidget *parent, ApplicationBackend *m_appBackend);
    ~AvailableView();

private:
    QApt::Backend *m_backend;
    ApplicationBackend *m_appBackend;

    QStackedWidget *m_viewStack;
    BreadcrumbWidget *m_breadcrumbWidget;
    CategoryView *m_categoryView;
    QHash<QModelIndex, QWidget *> m_viewHash;
    QStandardItemModel *m_categoryModel;
    QList<Category *> m_categoryList;
    ApplicationView *m_appView;
    ApplicationWidget *m_appWidget;

public Q_SLOTS:
    void setBackend(QApt::Backend *backend);

private Q_SLOTS:
    void populateCategories();
    void changeView(const QModelIndex &index);
    void activateItem(BreadcrumbItem *item);
    void showAppDetails(Application *app);
    void onViewDestroyed(QObject *view);
};

#endif
