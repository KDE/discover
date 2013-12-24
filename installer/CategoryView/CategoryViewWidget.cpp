/***************************************************************************
 *   Copyright © 2010-2012 Jonathan Thomas <echidnaman@kubuntu.org>        *
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
#include <QtWidgets/QVBoxLayout>

// KDE includes
#include <KCategorizedSortFilterProxyModel>
#include <KLocale>

// Libmuon includes
#include <Category/Category.h>
#include <Category/CategoryModel.h>

// Own includes
#include "ResourceView/ResourceViewWidget.h"
#include "BreadcrumbWidget/BreadcrumbItem.h"
#include "CategoryView.h"

CategoryViewWidget::CategoryViewWidget(QWidget *parent)
    : AbstractViewBase(parent)
    , m_searchView(0)
{
    m_searchable = true;
    m_categoryModel = new CategoryModel(this);

    m_categoryView = new CategoryView(this);

    m_layout->addWidget(m_categoryView);

    connect(m_categoryView, SIGNAL(activated(QModelIndex)),
           this, SLOT(onIndexActivated(QModelIndex)));
}


void CategoryViewWidget::setDisplayedCategory(Category* c)
{
    m_categoryModel->setDisplayedCategory(c);

    KCategorizedSortFilterProxyModel *proxy = new KCategorizedSortFilterProxyModel(this);
    proxy->setSourceModel(m_categoryModel);
    proxy->setCategorizedModel(true);
    m_categoryView->setModel(proxy);

    m_crumb->setText(c ? c->name() : i18n("Get Software"));
    m_crumb->setIcon(QIcon::fromTheme(c ? c->icon() : "applications-other"));
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
    Category *category = m_categoryModel->categoryForRow(index.row());

    if(!category->hasSubCategories()) {
        m_subView = new ResourceViewWidget(this);

        ResourceViewWidget *appView = static_cast<ResourceViewWidget *>(m_subView);
        appView->setFiltersFromCategory(category);
        appView->setTitle(category->name());
        appView->setIcon(QIcon::fromTheme(category->icon()));
        appView->setShouldShowTechnical(category->shouldShowTechnical());
    } else {
        m_subView = new CategoryViewWidget(this);

        CategoryViewWidget *subCatView = static_cast<CategoryViewWidget *>(m_subView);
        subCatView->setDisplayedCategory(category);
    }

    // Forward on to parent so that they can handle adding subviews to breadcrumb,
    // switching to subviews from the new subview, etc
    connect(m_subView, SIGNAL(registerNewSubView(AbstractViewBase*)),
            this, SIGNAL(registerNewSubView(AbstractViewBase*)));
    // Make sure we remove the index/widget association upon deletion
    connect(m_subView, SIGNAL(destroyed(QObject*)),
            this, SLOT(onSubViewDestroyed()));

    // Tell our parent that we can exist, so that they can forward it
    emit registerNewSubView(m_subView);
}

void CategoryViewWidget::search(const QString &text)
{
    if (text.size() < 2) {
        return;
    }

    if (!m_searchView) {
        m_searchView = new ResourceViewWidget(this);
        m_searchView->setTitle(i18nc("@label", "Search Results"));
        m_searchView->setIcon(QIcon::fromTheme("applications-other"));

        // Forward on to parent so that they can handle adding subviews to breadcrumb,
        // switching to subviews from the new subview, etc
        connect(m_searchView, SIGNAL(registerNewSubView(AbstractViewBase*)),
                this, SIGNAL(registerNewSubView(AbstractViewBase*)));
        // Make sure we clear the pointer upon deletion
        connect(m_searchView, SIGNAL(destroyed(QObject*)),
                this, SLOT(onSearchViewDestroyed()));

        // Tell our parent that we can exist, so that they can forward it
        emit registerNewSubView(m_searchView);
    } else {
        switchToSubView(m_searchView);
    }

    m_searchView->search(text);
}

void CategoryViewWidget::onSubViewDestroyed()
{
    auto iter = m_subViewHash.begin();
    while (iter != m_subViewHash.end()) {
        auto prev = iter;
        if (prev.value() == m_subView) {
            m_subViewHash.erase(prev);
            break; // Found our subview, guaranteed to be only one since we don't use insertMulti
        }
        ++iter;
    }
}

void CategoryViewWidget::onSearchViewDestroyed()
{
    m_searchView = nullptr;
}
