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
#include <QDebug>
#include <QCollator>

Q_GLOBAL_STATIC_WITH_ARGS(QVector<Category*>, s_categories, (CategoriesReader().populateCategories()))

CategoryModel::CategoryModel(QObject* parent)
    : QAbstractListModel(parent)
{
}

QHash< int, QByteArray > CategoryModel::roleNames() const
{
    QHash< int, QByteArray > names = QAbstractItemModel::roleNames();
    names[CategoryRole] = "category";
    return names;
}

bool lessThan(Category* a, Category* b)
{
    if (a->isAddons() == b->isAddons())
        return QCollator().compare(a->name(), b->name()) < 0;
    else
        return b->isAddons();
}

void CategoryModel::setCategories(const QList<Category *> &categoryList)
{
    beginResetModel();
    m_categories = categoryList;
    std::sort(m_categories.begin(), m_categories.end(), lessThan);
    endResetModel();
}

void CategoryModel::setCategories(const QVariantList& categoryList)
{
    QList<Category*> cats;
    foreach(const QVariant &cat, categoryList)
        cats += cat.value<Category*>();

    setCategories(cats);
}

void CategoryModel::categoryDeleted(QObject* cat)
{
    auto idx = m_categories.indexOf(static_cast<Category*>(cat));
    if (idx >= 0) {
        beginRemoveRows(QModelIndex(), idx, idx);
        m_categories.removeAt(idx);
        endRemoveRows();
    }
}

Category* CategoryModel::categoryForRow(int row)
{
    return qobject_cast<Category*>(m_categories.at(row));
}

static Category* recFindCategory(Category* root, const QString& name)
{
    if(root->name()==name)
        return root;
    else {
        const auto subs = root->subCategories();
        Q_FOREACH (Category* c, subs) {
            Category* ret = recFindCategory(c, name);
            if(ret)
                return ret;
        }
    }
    return nullptr;
}

QList<Category*> CategoryModel::rootCategories()
{
    return s_categories->toList();
}

Category* CategoryModel::findCategoryByName(const QString& name)
{
    const auto cats = *s_categories;
    Q_FOREACH (Category* cat, cats) {
        Category* ret = recFindCategory(cat, name);
        if(ret)
            return ret;
    }
    return nullptr;
}

void CategoryModel::blacklistPlugin(const QString& name)
{
    const QSet<QString> plugins = {name};
    for(auto it = s_categories->begin(), itEnd = s_categories->end(); it!=itEnd; ) {
        if ((*it)->blacklistPlugins(plugins)) {
            delete *it;
            it = s_categories->erase(it);
        } else
            ++it;
    }
}

void CategoryModel::resetCategories()
{
    setCategories(rootCategories());
}

int CategoryModel::rowCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : m_categories.count();
}

QVariant CategoryModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row()<0 || index.row() >= m_categories.count())
        return {};

    Category* c = m_categories[index.row()];
    Q_ASSERT(c);

    switch (role) {
        case Qt::DisplayRole:
            return c->name();
        case Qt::DecorationRole:
            return c->icon();
        case CategoryRole:
            return QVariant::fromValue<QObject*>(c);
    }
    return {};
}

QVariantList CategoryModel::categories() const
{
    QVariantList ret;
    for(Category* cat : m_categories) {
        ret.append(QVariant::fromValue<QObject*>(cat));
    }
    return ret;
}
