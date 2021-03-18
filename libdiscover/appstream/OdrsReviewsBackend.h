/*
 *   SPDX-FileCopyrightText: 2013 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *   SPDX-FileCopyrightText: 2017 Jan Grulich <jgrulich@redhat.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#ifndef ODRSREVIEWSBACKEND_H
#define ODRSREVIEWSBACKEND_H

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
    explicit OdrsReviewsBackend();
    ~OdrsReviewsBackend() override;

    QString userName() const override;
    void login() override
    {
    }
    void logout() override
    {
    }
    void registerAndLogin() override
    {
    }

    Rating *ratingForApplication(AbstractResource *app) const override;
    bool hasCredentials() const override
    {
        return false;
    }
    void deleteReview(Review *) override
    {
    }
    void fetchReviews(AbstractResource *app, int page = 1) override;
    bool isFetching() const override
    {
        return m_isFetching;
    }
    void submitReview(AbstractResource *, const QString &summary, const QString &description, const QString &rating) override;
    void flagReview(Review *, const QString &, const QString &) override
    {
    }
    void submitUsefulness(Review *review, bool useful) override;
    bool isResourceSupported(AbstractResource *res) const override;
    void emitRatingFetched(AbstractResourcesBackend *backend, const QList<AbstractResource *> &res) const;

private Q_SLOTS:
    void ratingsFetched(KJob *job);
    void reviewsFetched();
    void reviewSubmitted(QNetworkReply *reply);
    void usefulnessSubmitted();

Q_SIGNALS:
    void ratingsReady();

private:
    QNetworkAccessManager *nam();
    void parseRatings();
    void parseReviews(const QJsonDocument &document, AbstractResource *resource);

    QHash<QString, Rating *> m_ratings;
    bool m_isFetching;
    CachedNetworkAccessManager *m_delayedNam = nullptr;
};

#endif // ODRSREVIEWSBACKEND_H
