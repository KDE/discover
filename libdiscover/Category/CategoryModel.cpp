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
#include <utils.h>
#include <resources/ResourcesModel.h>

CategoryModel::CategoryModel(QObject* parent)
    : QObject(parent)
{
    connect(ResourcesModel::global(), &ResourcesModel::backendsChanged, this, &CategoryModel::populateCategories);
}

CategoryModel * CategoryModel::global()
{
    static CategoryModel *instance = nullptr;
    if (!instance) {
        instance = new CategoryModel;
    }
    return instance;
}

void CategoryModel::populateCategories()
{
    const auto backends = ResourcesModel::global()->backends();

    QVector<Category*> ret;
    CategoriesReader cr;
    Q_FOREACH (const auto backend, backends) {
        const QVector<Category*> cats = cr.loadCategoriesFile(backend);

        if(ret.isEmpty()) {
            ret = cats;
        } else {
            Q_FOREACH (Category* c, cats)
                Category::addSubcategory(ret, c);
        }
    }
    if (m_rootCategories != ret) {
        m_rootCategories = ret;
        Q_EMIT rootCategoriesChanged();
    }
}

QVariantList CategoryModel::rootCategoriesVL() const
{
    return kTransform<QVariantList>(m_rootCategories, [](Category* cat) {return qVariantFromValue<QObject*>(cat); });
}

void CategoryModel::blacklistPlugin(const QString &name)
{
    const bool ret = Category::blacklistPluginsInVector({name}, m_rootCategories);
    if (ret) {
        Q_EMIT rootCategoriesChanged();
    }
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

Category* CategoryModel::findCategoryByName(const QString& name) const
{
    for (Category* cat: m_rootCategories) {
        Category* ret = recFindCategory(cat, name);
        if(ret)
            return ret;
    }
    return nullptr;
}
