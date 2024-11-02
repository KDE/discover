/*
 *   SPDX-FileCopyrightText: 2013 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *   SPDX-FileCopyrightText: 2017 Jan Grulich <jgrulich@redhat.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include <ReviewsBackend/AbstractReviewsBackend.h>
#include <ReviewsBackend/ReviewsModel.h>

#include <QJsonDocument>
#include <QMap>
#include <QNetworkReply>

class KJob;
class AbstractResourcesBackend;
class CachedNetworkAccessManager;

class DISCOVERCOMMON_EXPORT OdrsReviewsBackend : public AbstractReviewsBackend
{
    Q_OBJECT
public:
    static QSharedPointer<OdrsReviewsBackend> global();
    explicit OdrsReviewsBackend();
    ~OdrsReviewsBackend() override;

    void login() override
    {
    }
    void logout() override
    {
    }
    void registerAndLogin() override
    {
    }

    Rating ratingForApplication(AbstractResource *resource) const override;
    bool hasCredentials() const override
    {
        return false;
    }
    bool supportsNameChange() const override
    {
        return true;
    }
    void deleteReview(Review *) override
    {
    }
    ReviewsJob *fetchReviews(AbstractResource *resource, int page = 1) override;
    bool isFetching() const override
    {
        return m_isFetching;
    }
    void flagReview(Review *, const QString &, const QString &) override
    {
    }
    void submitUsefulness(Review *review, bool useful) override;
    bool isResourceSupported(AbstractResource *resource) const override;
    void emitRatingFetched(AbstractResourcesBackend *backend, const QList<AbstractResource *> &resources) const;
    QString errorMessage() const override
    {
        return m_errorMessage;
    }

    const QList<Rating> &top() const
    {
        return m_current.top;
    }

private Q_SLOTS:
    void ratingsFetched(KJob *job);
    void usefulnessSubmitted();

Q_SIGNALS:
    void ratingsReady();

protected:
    ReviewsJob *
    sendReview(AbstractResource *resource, const QString &summary, const QString &reviewText, const QString &rating, const QString &userName) override;
    QString userName() const override;

private:
    void fetchRatings();
    void setFetching(bool fetching);
    QNetworkAccessManager *nam();
    void parseRatings();

    QString m_errorMessage;
    bool m_isFetching = false;
    CachedNetworkAccessManager *m_delayedNam = nullptr;
    QHash<QByteArray, ReviewsJob *> m_jobs;

    struct State {
        QHash<QString, Rating> ratings;
        QList<Rating> top;
    } m_current;
};
