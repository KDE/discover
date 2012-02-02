/***************************************************************************
 *   Copyright © 2012 Aleix Pol Gonzalez <aleixpol@kde.org>                *
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

#include "CategoryModel.h"
#include "CategoryView/Category.h"
#include <KIcon>
#include <KCategorizedSortFilterProxyModel>
#include <QDebug>

CategoryModel::CategoryModel(QObject* parent)
    : QStandardItemModel(parent)
{
    QHash< int, QByteArray > names = roleNames();
    names[CategoryTypeRole] = "categoryType";
    names[AndOrFilterRole] = "andOrFilter";
    names[NotFilterRole] = "notFilter";
    names[CategoryRole] = "category";
    setRoleNames(names);
}

CategoryModel::~CategoryModel()
{
}

void CategoryModel::setCategories(const QList<Category *> &categoryList,
                                  const QString &rootName)
{
    qDeleteAll(m_categoryList);
    m_categoryList = categoryList;
    foreach (Category *category, m_categoryList) {
        if(!category->parent())
            category->setParent(this);
        QStandardItem *categoryItem = new QStandardItem;
        categoryItem->setText(category->name());
        categoryItem->setIcon(KIcon(category->icon()));
        categoryItem->setEditable(false);
        categoryItem->setData(rootName, KCategorizedSortFilterProxyModel::CategoryDisplayRole);
        categoryItem->setData(qVariantFromValue<QObject*>(category), CategoryRole);

        if (category->hasSubCategories()) {
            categoryItem->setData(SubCatType, CategoryTypeRole);
        } else {
            categoryItem->setData(CategoryType, CategoryTypeRole);
        }

        appendRow(categoryItem);
    }
}

Category* CategoryModel::categoryForIndex(int row)
{
    return m_categoryList.at(row);
}

void CategoryModel::populateCategories(const QString& rootName)
{
    setCategories(Category::populateCategories(), rootName);
}

void CategoryModel::setSubcategories(Category* c)
{
    setCategories(c->subCategories(), c->name());
}
