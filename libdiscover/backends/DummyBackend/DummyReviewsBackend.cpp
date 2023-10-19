/*
 *   SPDX-FileCopyrightText: 2013 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "DummyReviewsBackend.h"
#include "DummyBackend.h"
#include "DummyResource.h"
#include <QDebug>
#include <QRandomGenerator>
#include <QTimer>
#include <ReviewsBackend/Rating.h>
#include <ReviewsBackend/Review.h>
#include <resources/AbstractResource.h>

DummyReviewsBackend::DummyReviewsBackend(DummyBackend *parent)
    : AbstractReviewsBackend(parent)
{
}

DummyReviewsBackend::~DummyReviewsBackend() noexcept
{
    qDeleteAll(m_ratings);
}

void DummyReviewsBackend::fetchReviews(AbstractResource *app, int page)
{
    if (page >= 5)
        return;

    QList<ReviewPtr> review;
    for (int i = 0; i < 33; i++) {
        review += ReviewPtr(new Review(app->name(),
                                       app->packageName(),
                                       QStringLiteral("en_US"),
                                       QStringLiteral("good morning"),
                                       QStringLiteral("the morning is very good"),
                                       QStringLiteral("dummy"),
                                       QDateTime(),
                                       true,
                                       page + i,
                                       i % 5,
                                       1,
                                       1,
                                       app->packageName()));
    }
    Q_EMIT reviewsReady(app, review, false);
}

Rating *DummyReviewsBackend::ratingForApplication(AbstractResource *app) const
{
    return m_ratings[app];
}

void DummyReviewsBackend::initialize()
{
    int i = 11;
    DummyBackend *b = qobject_cast<DummyBackend *>(parent());
    const auto resources = b->resources();
    for (DummyResource *app : resources) {
        if (m_ratings.contains(app))
            continue;

        int ratings[] = {0, 0, 0, 0, 0, QRandomGenerator::global()->bounded(0, 10)};
        Rating *rating = new Rating(app->packageName(), ++i, ratings);
        m_ratings.insert(app, rating);
        Q_EMIT app->ratingFetched();
    }
    Q_EMIT ratingsReady();
}

void DummyReviewsBackend::submitUsefulness(Review *r, bool useful)
{
    qDebug() << "usefulness..." << r->applicationName() << r->reviewer() << useful;
    r->setUsefulChoice(useful ? ReviewsModel::Yes : ReviewsModel::No);
}

void DummyReviewsBackend::sendReview(AbstractResource *res, const QString &a, const QString &b, const QString &c, const QString &d)
{
    qDebug() << "dummy submit review" << res->name() << a << b << c << d;
}

bool DummyReviewsBackend::isResourceSupported(AbstractResource * /*res*/) const
{
    return true;
}
