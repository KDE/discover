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

#include "CategoryViewWidget.h"

// Qt includes
#include <QStandardItemModel>
#include <QtGui/QVBoxLayout>

// KDE includes
#include <KCategorizedSortFilterProxyModel>
#include <KIcon>
#include <KLocale>
#include <KDebug>

// LibQApt includes
#include <LibQApt/Backend>

// Own includes
#include "../ApplicationBackend.h"
#include "../ApplicationModel/ApplicationViewWidget.h"
#include "../BreadcrumbWidget/BreadcrumbItem.h"
#include "Category.h"
#include "CategoryView.h"

CategoryViewWidget::CategoryViewWidget(QWidget *parent, ApplicationBackend *appBackend)
    : AbstractViewBase(parent)
    , m_backend(0)
    , m_appBackend(appBackend)
    , m_searchView(0)
{
    m_searchable = true;
    m_categoryModel = new QStandardItemModel(this);

    m_categoryView = new CategoryView(this);

    m_layout->addWidget(m_categoryView);

    connect(m_categoryView, SIGNAL(activated(const QModelIndex &)),
           this, SLOT(onIndexActivated(const QModelIndex &)));
}

CategoryViewWidget::~CategoryViewWidget()
{
}

void CategoryViewWidget::setBackend(QApt::Backend *backend)
{
    m_backend = backend;
}

void CategoryViewWidget::setCategories(const QList<Category *> &categoryList,
                                       const QString &rootName,
                                       const QIcon &icon)
{
    m_categoryList = categoryList;
    foreach (Category *category, m_categoryList) {
        QStandardItem *categoryItem = new QStandardItem;
        categoryItem->setText(category->name());
        categoryItem->setIcon(KIcon(category->icon()));
        categoryItem->setEditable(false);
        categoryItem->setData(rootName, KCategorizedSortFilterProxyModel::CategoryDisplayRole);

        if (category->hasSubCategories()) {
            categoryItem->setData(SubCatType, CategoryTypeRole);
        } else {
            categoryItem->setData(CategoryType, CategoryTypeRole);
        }

        m_categoryModel->appendRow(categoryItem);
    }

    KCategorizedSortFilterProxyModel *proxy = new KCategorizedSortFilterProxyModel(this);
    proxy->setSourceModel(m_categoryModel);
    proxy->setCategorizedModel(true);
    m_categoryView->setModel(proxy);

    m_crumb->setText(rootName);
    m_crumb->setIcon(icon);
    m_crumb->setAssociatedView(this);
}

void CategoryViewWidget::onIndexActivated(const QModelIndex &index)
{
    AbstractViewBase *view = m_subViewHash.value(index);

    // If a view already exists, switch to it without disturbing things
    if (view && m_subView == view) {
        emit switchToSubView(view);
        return;
    }

    // Otherwise we have to create a new view
    Category *category = m_categoryList.at(index.row());

    switch (index.data(CategoryTypeRole).toInt()) {
    case CategoryType: { // Displays the apps in a category
        m_subView = new ApplicationViewWidget(this, m_appBackend);

        ApplicationViewWidget *appView = static_cast<ApplicationViewWidget *>(m_subView);
        appView->setBackend(m_backend);
        appView->setFiltersFromCategory(category);
        appView->setTitle(category->name());
        appView->setIcon(KIcon(category->icon()));
    }
        break;
    case SubCatType: { // Displays the subcategories of a category
        m_subView = new CategoryViewWidget(this, m_appBackend);

        CategoryViewWidget *subCatView = static_cast<CategoryViewWidget *>(m_subView);
        subCatView->setBackend(m_backend);
        subCatView->setCategories(category->subCategories(), category->name(),
                                  KIcon(category->icon()));
    }
        break;
    }

    // Forward on to parent so that they can handle adding subviews to breadcrumb,
    // switching to subviews from the new subview, etc
    connect(m_subView, SIGNAL(registerNewSubView(AbstractViewBase *)),
            this, SIGNAL(registerNewSubView(AbstractViewBase *)));
    // Make sure we remove the index/widget association upon deletion
    connect(m_subView, SIGNAL(destroyed(QObject *)),
            this, SLOT(onSubViewDestroyed()));

    // Tell our parent that we can exist, so that they can forward it
    emit registerNewSubView(m_subView);
}

void CategoryViewWidget::search(const QString &text)
{
    if (!m_searchView) {
        m_searchView = new ApplicationViewWidget(this, m_appBackend);
        m_searchView->setBackend(m_backend);
        m_searchView->setTitle(i18nc("@label", "Search Results"));
        m_searchView->setIcon(KIcon("applications-other"));

        // Forward on to parent so that they can handle adding subviews to breadcrumb,
        // switching to subviews from the new subview, etc
        connect(m_searchView, SIGNAL(registerNewSubView(AbstractViewBase *)),
                this, SIGNAL(registerNewSubView(AbstractViewBase *)));
        // Make sure we remove the index/widget association upon deletion
        connect(m_searchView, SIGNAL(destroyed(QObject *)),
                this, SLOT(onSubViewDestroyed()));

        // Tell our parent that we can exist, so that they can forward it
        emit registerNewSubView(m_searchView);
    } else {
        switchToSubView(m_searchView);
    }

    m_searchView->search(text);
}

void CategoryViewWidget::onSubViewDestroyed()
{
    m_subViewHash.remove(m_subViewHash.key(m_subView));
}

#include "CategoryViewWidget.moc"
