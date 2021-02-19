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
    bool supportsNameChange() const override
    {
        return true;
    }
    void deleteReview(Review *) override
    {
    }
    void fetchReviews(AbstractResource *app, int page = 1) override;
    bool isFetching() const override
    {
        return m_isFetching;
    }
    void flagReview(Review *, const QString &, const QString &) override
    {
    }
    void submitUsefulness(Review *review, bool useful) override;
    bool isResourceSupported(AbstractResource *res) const override;
    void emitRatingFetched(AbstractResourcesBackend *backend, const QList<AbstractResource *> &res) const;
    QString errorMessage() const override
    {
        return m_errorMessage;
    }

    QVector<Rating *> top() const
    {
        return m_top;
    }

private Q_SLOTS:
    void ratingsFetched(KJob *job);
    void reviewsFetched();
    void reviewSubmitted(QNetworkReply *reply);
    void usefulnessSubmitted();

Q_SIGNALS:
    void ratingsReady();

protected:
    void sendReview(AbstractResource *app, const QString &summary, const QString &review_text, const QString &rating, const QString &userName) override;
    QString userName() const override;

private:
    void fetchRatings();
    void setFetching(bool);
    QNetworkAccessManager *nam();
    void parseRatings();
    void parseReviews(const QJsonDocument &document, AbstractResource *resource);

    QString m_errorMessage;
    QHash<QString, Rating *> m_ratings;
    bool m_isFetching = false;
    CachedNetworkAccessManager *m_delayedNam = nullptr;

    QVector<Rating *> m_top;
};

#endif // ODRSREVIEWSBACKEND_H
