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
#include <QQmlEngine>
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
    if (!ResourcesModel::global()->backends().isEmpty()) {
        populateCategories();
    }
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

    QList<std::shared_ptr<Category>> ret;
    CategoriesReader cr;
    for (const auto backend : backends) {
        if (!backend->isValid())
            continue;

        const QList<std::shared_ptr<Category>> cats = cr.loadCategoriesFile(backend);

        if (ret.isEmpty()) {
            ret = cats;
        } else {
            for (const std::shared_ptr<Category> &c : cats)
                Category::addSubcategory(ret, c);
        }
    }
    if (m_rootCategories != ret) {
        m_rootCategories = ret;
        m_rootCategoriesChanged->start();
    }
}

const QList<std::shared_ptr<Category>> &CategoryModel::rootCategories() const
{
    return m_rootCategories;
}

void CategoryModel::blacklistPlugin(const QString &name)
{
    const bool ret = Category::blacklistPluginsInVector({name}, m_rootCategories);
    if (ret) {
        m_rootCategoriesChanged->start();
    }
}

static std::shared_ptr<Category> recFindCategory(std::shared_ptr<Category> root, const QString &name)
{
    if (root->untranslatedName() == name)
        return root;
    else {
        const auto &subs = root->subCategories();
        for (const std::shared_ptr<Category> &c : subs) {
            std::shared_ptr<Category> ret = recFindCategory(c, name);
            if (ret)
                return ret;
        }
    }
    return nullptr;
}

QObject *CategoryModel::get(const std::shared_ptr<Category> &ptr)
{
    QQmlEngine::setObjectOwnership(ptr.get(), QQmlEngine::CppOwnership);
    return ptr.get();
}

std::shared_ptr<Category> CategoryModel::findCategoryByName(const QString &name) const
{
    for (const std::shared_ptr<Category> &cat : m_rootCategories) {
        std::shared_ptr<Category> ret = recFindCategory(cat, name);
        if (ret)
            return ret;
    }
    if (!m_rootCategories.isEmpty()) {
        qDebug() << "could not find category" << name << m_rootCategories;
    }
    return nullptr;
}

#include "moc_CategoryModel.cpp"
