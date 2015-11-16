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

#include <resources/ResourcesModel.h>

#include <QDebug>

class AllCategories : public QObject
{
Q_OBJECT
public:
    AllCategories() {
        connect(ResourcesModel::global(), &ResourcesModel::backendsChanged, this, &AllCategories::updateCategories, Qt::QueuedConnection);
        updateCategories();
    }

    static AllCategories* global() {
        static AllCategories* s_self = new AllCategories;
        return s_self;
    }

    void updateCategories() {
        QSet<AbstractResourcesBackend*> newBackends = ResourcesModel::global()->backends().toList().toSet();
        if (newBackends == m_backends)
            return;
        m_backends = newBackends;

        qDeleteAll(m_categories);
        m_categories = CategoriesReader().populateCategories();

        Q_EMIT categoriesChanged();
    }

    void blacklist(const QString &name) {
        const QSet<QString> plugins = {name};
        for(QList<Category*>::iterator it = m_categories.begin(), itEnd = m_categories.end(); it!=itEnd; ) {
            if ((*it)->blacklistPlugins(plugins)) {
                delete *it;
                it = m_categories.erase(it);
            } else
                ++it;
        }
    }

    QList<Category*> categories() const { return m_categories; }

Q_SIGNALS:
    void categoriesChanged();

private:
    QSet<AbstractResourcesBackend*> m_backends;
    QList<Category*> m_categories;
};

CategoryModel::CategoryModel(QObject* parent)
    : QStandardItemModel(parent)
    , m_currentCategory(nullptr)
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
    if (m_currentCategory == c && (c || rowCount()>0))
        return;

    m_currentCategory = c;
    disconnect(AllCategories::global(), &AllCategories::categoriesChanged, this, &CategoryModel::resetCategories);
    if(c)
        setCategories(c->subCategories(), c->name());
    else {
        resetCategories();
        connect(AllCategories::global(), &AllCategories::categoriesChanged, this, &CategoryModel::resetCategories);
    }

    categoryChanged(c);
}

void CategoryModel::resetCategories()
{
    setCategories(AllCategories::global()->categories(), QString());
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
        Q_FOREACH (Category* c, subs) {
            Category* ret = recFindCategory(c, name);
            if(ret)
                return ret;
        }
    }
    return nullptr;
}

Category* CategoryModel::findCategoryByName(const QString& name)
{
    const QList<Category*> cats = AllCategories::global()->categories();
    Q_FOREACH (Category* cat, cats) {
        Category* ret = recFindCategory(cat, name);
        if(ret)
            return ret;
    }
    return nullptr;
}

void CategoryModel::blacklistPlugin(const QString& name)
{
    AllCategories::global()->blacklist(name);
}

#include "CategoryModel.moc"
