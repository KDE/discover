/*
 *   SPDX-FileCopyrightText: 2012 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

// Own includes
#include "CategoryModel.h"
#include "CategoriesReader.h"
#include "libdiscover_debug.h"
#include <QCollator>
#include <resources/ResourcesModel.h>
#include <utils.h>

CategoryModel::CategoryModel(QObject *parent)
    : QObject(parent)
{
    QTimer *t = new QTimer(this);
    t->setInterval(0);
    t->setSingleShot(true);
    connect(t, &QTimer::timeout, this, &CategoryModel::populateCategories);
    connect(ResourcesModel::global(), &ResourcesModel::backendsChanged, t, QOverload<>::of(&QTimer::start));

    // Use a timer so to compress the rootCategoriesChanged signal
    // It generally triggers when KNS is unavailable at large (as explained in bug 454442)
    m_rootCategoriesChanged = new QTimer(this);
    m_rootCategoriesChanged->setInterval(0);
    m_rootCategoriesChanged->setSingleShot(true);
    connect(m_rootCategoriesChanged, &QTimer::timeout, this, &CategoryModel::rootCategoriesChanged);
}

CategoryModel *CategoryModel::global()
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

    QVector<Category *> ret;
    CategoriesReader cr;
    for (const auto backend : backends) {
        if (!backend->isValid())
            continue;

        const QVector<Category *> cats = cr.loadCategoriesFile(backend);

        if (ret.isEmpty()) {
            ret = cats;
        } else {
            for (Category *c : cats)
                Category::addSubcategory(ret, c);
        }
    }
    if (m_rootCategories != ret) {
        m_rootCategories = ret;
        m_rootCategoriesChanged->start();
    }
}

QVariantList CategoryModel::rootCategoriesVL() const
{
    return kTransform<QVariantList>(m_rootCategories, [](Category *cat) {
        return QVariant::fromValue<QObject *>(cat);
    });
}

void CategoryModel::blacklistPlugin(const QString &name)
{
    const bool ret = Category::blacklistPluginsInVector({name}, m_rootCategories);
    if (ret) {
        m_rootCategoriesChanged->start();
    }
}

static Category *recFindCategory(Category *root, const QString &name)
{
    if (root->name() == name)
        return root;
    else {
        const auto subs = root->subCategories();
        for (Category *c : subs) {
            Category *ret = recFindCategory(c, name);
            if (ret)
                return ret;
        }
    }
    return nullptr;
}

Category *CategoryModel::findCategoryByName(const QString &name) const
{
    for (Category *cat : m_rootCategories) {
        Category *ret = recFindCategory(cat, name);
        if (ret)
            return ret;
    }
    qDebug() << "could not find category" << name << m_rootCategories;
    return nullptr;
}
