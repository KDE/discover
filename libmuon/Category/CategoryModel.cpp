/***************************************************************************
 *   Copyright © 2012 Aleix Pol Gonzalez <aleixpol@blue-systems.com>       *
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

// Own includes
#include "CategoryModel.h"
#include "Category.h"
#include "CategoriesReader.h"

// KDE includes
#include <KCategorizedSortFilterProxyModel>

Q_GLOBAL_STATIC_WITH_ARGS(QList<Category*>, s_categories, (CategoriesReader().populateCategories()))

CategoryModel::CategoryModel(QObject* parent)
    : QStandardItemModel(parent)
{
}

QHash< int, QByteArray > CategoryModel::roleNames() const
{
    QHash< int, QByteArray > names = QAbstractItemModel::roleNames();
    names[CategoryRole] = "category";
    return names;
}

void CategoryModel::setCategories(const QList<Category *> &categoryList, const QString &rootName)
{
    clear();

    invisibleRootItem()->removeRows(0, invisibleRootItem()->rowCount());
    foreach (Category *category, categoryList) {
        QStandardItem *categoryItem = new QStandardItem;
        categoryItem->setText(category->name());
        categoryItem->setIcon(QIcon::fromTheme(category->icon()));
        categoryItem->setEditable(false);
        categoryItem->setData(rootName, KCategorizedSortFilterProxyModel::CategoryDisplayRole);
        categoryItem->setData(qVariantFromValue<QObject*>(category), CategoryRole);
        connect(category, &QObject::destroyed, this, &CategoryModel::categoryDeleted);

        appendRow(categoryItem);
    }
}

void CategoryModel::categoryDeleted(QObject* cat)
{
    for(int i=0; i<rowCount(); ++i) {
        if (cat == item(i)->data(CategoryRole).value<QObject*>()) {
            removeRow(i);
        }
    }
}

Category* CategoryModel::categoryForRow(int row)
{
    return qobject_cast<Category*>(item(row)->data(CategoryRole).value<QObject*>());
}

void CategoryModel::setDisplayedCategory(Category* c)
{
    m_currentCategory = c;
    if(c)
        setCategories(c->subCategories(), c->name());
    else
        setCategories(*s_categories, QString());
}

Category* CategoryModel::displayedCategory() const
{
    return m_currentCategory;
}

static Category* recFindCategory(Category* root, const QString& name)
{
    if(root->name()==name)
        return root;
    else if(root->hasSubCategories()) {
        const QList<Category*> subs = root->subCategories();
        for(Category* c : subs) {
            Category* ret = recFindCategory(c, name);
            if(ret)
                return ret;
        }
    }
    return 0;
}

Category* CategoryModel::findCategoryByName(const QString& name)
{
    const QList<Category*> cats = *s_categories;
    for(Category* cat : cats) {
        Category* ret = recFindCategory(cat, name);
        if(ret)
            return ret;
    }
    return 0;
}

void CategoryModel::blacklistPlugin(const QString& name)
{
    const QSet<QString> plugins = {name};
    for(QList<Category*>::iterator it = s_categories->begin(), itEnd = s_categories->end(); it!=itEnd; ) {
        if ((*it)->blacklistPlugins(plugins)) {
            delete *it;
            it = s_categories->erase(it);
        } else
            ++it;
    }
}
