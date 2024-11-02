/*
 *   SPDX-FileCopyrightText: 2020 Alexey Minnekhanov <alexey.min@gmail.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#ifndef ALPINEAPKREVIEWSBACKEND_H
#define ALPINEAPKREVIEWSBACKEND_H

#include "ReviewsBackend/AbstractReviewsBackend.h"

class AlpineApkBackend;

class AlpineApkReviewsBackend : public AbstractReviewsBackend
{
    Q_OBJECT

public:
    explicit AlpineApkReviewsBackend(AlpineApkBackend *parent = nullptr);

    void login() override
    {
    }
    void logout() override
    {
    }
    void registerAndLogin() override
    {
    }

    Q_SCRIPTABLE Rating ratingForApplication(AbstractResource *) const override;
    bool hasCredentials() const override
    {
        return false;
    }
    void deleteReview(Review *) override
    {
    }
    ReviewsJob *fetchReviews(AbstractResource *app, int page = 1) override;
    bool isFetching() const override
    {
        return false;
    }
    bool isReviewable() const override
    {
        return false;
    }
    void flagReview(Review *, const QString &, const QString &) override
    {
    }
    void submitUsefulness(Review *, bool) override
    {
    }
    bool isResourceSupported(AbstractResource *) const override
    {
        return false;
    }
    bool supportsNameChange() const override
    {
        return false;
    }

protected:
    ReviewsJob *sendReview(AbstractResource *, const QString &, const QString &, const QString &, const QString &) override
    {
        auto ret = new ReviewsJob;
        ret->deleteLater();
        return ret;
    }
    QString userName() const override
    {
        return QStringLiteral("dummy");
    }
};

#endif // ALPINEAPKREVIEWSBACKEND_H
