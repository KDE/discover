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
}

void DummyReviewsBackend::fetchReviews(AbstractResource *resource, int page)
{
    if (page >= 5) {
        return;
    }

    QVector<ReviewPtr> review;
    for (int i = 0; i < 33; i++) {
        review += ReviewPtr(new Review(resource->name(),
                                       resource->packageName(),
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
                                       3,
                                       resource->packageName()));
    }
    Q_EMIT reviewsReady(resource, review, false);
}

Rating DummyReviewsBackend::ratingForApplication(AbstractResource *resource) const
{
    return m_ratings[resource];
}

void DummyReviewsBackend::initialize()
{
    int i = 11;
    const auto backend = qobject_cast<DummyBackend *>(parent());
    const auto resources = backend->resources();
    for (const auto resource : resources) {
        if (m_ratings.contains(resource)) {
            continue;
        }

        int ratings[] = {0, 0, 0, 0, 0, QRandomGenerator::global()->bounded(0, 10)};
        auto rating = Rating(resource->packageName(), ++i, ratings);
        m_ratings.insert(resource, rating);
        Q_EMIT resource->ratingFetched();
    }
    Q_EMIT ratingsReady();
}

void DummyReviewsBackend::submitUsefulness(Review *review, bool useful)
{
    qDebug() << "usefulness..." << review->applicationName() << review->reviewer() << useful;
    review->setUsefulChoice(useful ? ReviewsModel::Yes : ReviewsModel::No);
}

void DummyReviewsBackend::sendReview(AbstractResource *resource,
                                     const QString &summary,
                                     const QString &reviewText,
                                     const QString &rating,
                                     const QString &userName)
{
    qDebug() << "dummy submit review" << resource->name() << summary << reviewText << rating << userName;
}

bool DummyReviewsBackend::isResourceSupported(AbstractResource *resource) const
{
    Q_UNUSED(resource)
    return true;
}

#include "moc_DummyReviewsBackend.cpp"
