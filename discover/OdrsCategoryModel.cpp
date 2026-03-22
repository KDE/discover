/*
 *   SPDX-FileCopyrightText: 2022 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "OdrsCategoryModel.h"
#include <Category/CategoryModel.h>
#include <ReviewsBackend/Rating.h>
#include <appstream/OdrsReviewsBackend.h>
#include <utils.h>

using namespace Qt::StringLiterals;

OdrsCategoryModel::OdrsCategoryModel()
{
    auto backend = OdrsReviewsBackend::global();
    connect(backend.get(), &OdrsReviewsBackend::ratingsReady, this, &OdrsCategoryModel::refresh);
}

void OdrsCategoryModel::refresh()
{
    if (!m_initialized)
        return;

    qDebug() << "refreshing..." << m_category << m_pageSize << m_initialized;
    OdrsReviewsBackend::global()->topCategory(m_category, m_pageSize).then(this, [this](const QList<Rating> &top) {
        for (auto r : top)
            qDebug() << "toppp" << r.packageName();
        setUris(kTransform<QVector<QUrl>>(top, [](const Rating &rating) {
            return QUrl("appstream://"_L1 + rating.packageName());
        }));
    });
}

void OdrsCategoryModel::setCategoryName(const QString &categoryName)
{
    if (auto category = CategoryModel::global()->findCategoryByName(categoryName)) {
        setCategory(category);
    } else {
        qDebug() << "looking up wrong category or too early" << categoryName;
        auto f = [this, categoryName] {
            setCategory(CategoryModel::global()->findCategoryByName(categoryName));
        };
        delete m_delayed;
        m_delayed = new OneTimeAction(f, this);
        connect(CategoryModel::global(), &CategoryModel::rootCategoriesChanged, m_delayed.get(), &OneTimeAction::trigger);
    }
}

void OdrsCategoryModel::setCategory(const std::shared_ptr<Category> &category)
{
    if (m_category == category) {
        return;
    }
    m_category = category;
    refresh();
}

#include "moc_OdrsCategoryModel.cpp"
