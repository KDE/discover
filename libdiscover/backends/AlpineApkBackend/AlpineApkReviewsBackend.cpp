/*
 *   SPDX-FileCopyrightText: 2020 Alexey Minnekhanov <alexey.min@gmail.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "AlpineApkReviewsBackend.h"
#include "AlpineApkBackend.h"
#include "resources/AbstractResource.h"

AlpineApkReviewsBackend::AlpineApkReviewsBackend(AlpineApkBackend *parent)
    : AbstractReviewsBackend(parent)
{
}

Rating AlpineApkReviewsBackend::ratingForApplication(AbstractResource *) const
{
    return Rating();
}

ReviewsJob *AlpineApkReviewsBackend::fetchReviews(AbstractResource *app, int page)
{
    Q_UNUSED(app);
    Q_UNUSED(page);

    auto ret = new ReviewsJob;
    static const QVector<ReviewPtr> reviews;
    Q_EMIT ret->reviewsReady(reviews, false);
    ret->deleteLater();
    return ret;
}
