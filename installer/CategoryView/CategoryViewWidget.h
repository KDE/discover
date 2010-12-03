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

#ifndef CATEGORYVIEWWIDGET_H
#define CATEGORYVIEWWIDGET_H

#include "../AbstractViewBase.h"

#include <QModelIndex>
#include <QtCore/QList>

class QIcon;
class QStandardItemModel;
class QString;

class ApplicationBackend;
class ApplicationViewWidget;
class Category;
class CategoryView;

namespace QApt {
    class Backend;
}

enum CatViewType {
    /// An invalid type
    InvalidType = 0,
    /// An AppView since there are no sub-cats
    CategoryType = 1,
    /// A SubCategoryView
    SubCatType = 2
};

enum CategoryModelRole {
    CategoryTypeRole = Qt::UserRole + 1,
    AndOrFilterRole = Qt::UserRole + 2,
    NotFilterRole = Qt::UserRole + 3
};

class CategoryViewWidget : public AbstractViewBase
{
    Q_OBJECT
public:
    CategoryViewWidget(QWidget *parent, ApplicationBackend *appBackend);
    ~CategoryViewWidget();

    void setCategories(const QList<Category *> &categoryList,
                       const QString &rootText,
                       const QIcon &rootIcon);
    virtual void search(const QString &text);

private:
    QApt::Backend *m_backend;
    ApplicationBackend *m_appBackend;
    QStandardItemModel *m_categoryModel;
    QList<Category *> m_categoryList;
    QHash<QModelIndex, AbstractViewBase *> m_subViewHash;

    CategoryView *m_categoryView;
    AbstractViewBase *m_subView;
    ApplicationViewWidget *m_searchView;

public Q_SLOTS:
    void setBackend(QApt::Backend *backend);

private Q_SLOTS:
    void onIndexActivated(const QModelIndex &index);
    void onSubViewDestroyed();

Q_SIGNALS:
    void switchToSubView(AbstractViewBase *view);
    void registerNewSubView(AbstractViewBase *view);
};

#endif
