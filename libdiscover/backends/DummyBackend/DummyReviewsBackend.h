/*
 *   SPDX-FileCopyrightText: 2013 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include <QMap>
#include <ReviewsBackend/AbstractReviewsBackend.h>
#include <ReviewsBackend/ReviewsModel.h>

class DummyBackend;
class DummyReviewsBackend : public AbstractReviewsBackend
{
    Q_OBJECT
public:
    explicit DummyReviewsBackend(DummyBackend *parent = nullptr);
    ~DummyReviewsBackend() override;

    QString userName() const override
    {
        return QStringLiteral("dummy");
    }
    void login() override
    {
    }
    void logout() override
    {
    }
    void registerAndLogin() override
    {
    }

    Rating *ratingForApplication(AbstractResource *resource) const override;
    bool hasCredentials() const override
    {
        return false;
    }
    void deleteReview(Review *) override
    {
    }
    void fetchReviews(AbstractResource *resource, int page = 1) override;
    bool isFetching() const override
    {
        return false;
    }

    void flagReview(Review *, const QString &, const QString &) override
    {
    }
    void submitUsefulness(Review *, bool) override;

    void initialize();
    bool isResourceSupported(AbstractResource *resource) const override;

Q_SIGNALS:
    void ratingsReady();

protected:
    void sendReview(AbstractResource *resource, const QString &summary, const QString &reviewText, const QString &rating, const QString &userName) override;

private:
    QHash<AbstractResource *, Rating *> m_ratings;
};
