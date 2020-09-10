/*
 *   SPDX-FileCopyrightText: 2012 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

// Own includes
#include "CategoryModel.h"
#include "Category.h"
#include "CategoriesReader.h"
#include "libdiscover_debug.h"
#include <QCollator>
#include <utils.h>
#include <resources/ResourcesModel.h>

CategoryModel::CategoryModel(QObject* parent)
    : QObject(parent)
{
    QTimer* t = new QTimer(this);
    t->setInterval(0);
    t->setSingleShot(true);
    connect(t, &QTimer::timeout, this, &CategoryModel::populateCategories);
    connect(ResourcesModel::global(), &ResourcesModel::backendsChanged, t, QOverload<>::of(&QTimer::start));
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
        if (!backend->isValid())
            continue;

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
    return kTransform<QVariantList>(m_rootCategories, [](Category* cat) {return QVariant::fromValue<QObject*>(cat); });
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
