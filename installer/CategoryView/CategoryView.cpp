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

#include "CategoryView.h"

// Qt includes
#include <QStandardItemModel>

// KDE includes
#include <KCategorizedSortFilterProxyModel>
#include <KDialog>
#include <KCategoryDrawer>
#include <KFileItemDelegate>
#include <KDE/KLocale>

// Own includes
#include "Category.h"
#include "CategoryDrawer.h"

CategoryView::CategoryView(QWidget *parent)
    : KCategorizedView(parent)
{
    CategoryDrawer *drawer = new CategoryDrawer();

    setSelectionMode(QAbstractItemView::SingleSelection);
    setSpacing(KDialog::spacingHint());
    setResizeMode(QListView::Adjust);
    setWordWrap(true);
    setCategoryDrawer(drawer);
    setViewMode(QListView::IconMode);
    setMouseTracking( true );
    viewport()->setAttribute( Qt::WA_Hover );

    KFileItemDelegate *delegate = new KFileItemDelegate(this);
    delegate->setWrapMode(QTextOption::WordWrap);
    setItemDelegate(delegate);

    m_categoryModel = new QStandardItemModel(this);

    KCategorizedSortFilterProxyModel *proxy = new KCategorizedSortFilterProxyModel(this);
    proxy->setSourceModel(m_categoryModel);
    proxy->setCategorizedModel(true);
    proxy->sort(0);
    setModel(proxy);
}

void CategoryView::setModel(QAbstractItemModel *model)
{
    //icon stuff ripped from System Settings trunk
    KCategorizedView::setModel(model);
    int maxWidth = -1;
    int maxHeight = -1;
    for (int i = 0; i < model->rowCount(); ++i) {
        const QModelIndex index = model->index(i, modelColumn(), rootIndex());
        const QSize size = sizeHintForIndex(index);
        maxWidth = qMax(maxWidth, size.width());
        maxHeight = qMax(maxHeight, size.height());
    }
    setGridSize(QSize(maxWidth, maxHeight ));
    static_cast<KFileItemDelegate*>(itemDelegate())->setMaximumSize(QSize(maxWidth, maxHeight));
}

void CategoryView::setCategories(const QList<Category *> &categoryList)
{
    foreach (Category *category, categoryList) {
        QStandardItem *categoryItem = new QStandardItem;
        categoryItem->setText(category->name());
        categoryItem->setIcon(KIcon(category->icon()));
        categoryItem->setEditable(false);
        categoryItem->setData(i18n("Categories"), KCategorizedSortFilterProxyModel::CategoryDisplayRole);

        if (category->hasSubCategories()) {
            categoryItem->setData(SubCatType, CategoryTypeRole);
        } else {
            categoryItem->setData(CategoryType, CategoryTypeRole);
        }

        m_categoryModel->appendRow(categoryItem);
    }
}

#include "CategoryView.moc"
