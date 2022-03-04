/*
 *   SPDX-FileCopyrightText: 2012 Aleix Pol Gonzalez <aleixpol@blue-systems.com
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#ifndef ABSTRACTREVIEWSBACKEND_H
#define ABSTRACTREVIEWSBACKEND_H

#include <QObject>

#include "Review.h"
#include "ReviewsModel.h"
class Rating;
class AbstractResource;

class DISCOVERCOMMON_EXPORT AbstractReviewsBackend : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool isReviewable READ isReviewable CONSTANT)
    Q_PROPERTY(bool hasCredentials READ hasCredentials NOTIFY loginStateChanged)
    Q_PROPERTY(QString userName READ userName NOTIFY loginStateChanged)
public:
    explicit AbstractReviewsBackend(QObject *parent = nullptr);

    virtual QString userName() const = 0;
    virtual bool hasCredentials() const = 0;

    Q_SCRIPTABLE virtual Rating *ratingForApplication(AbstractResource *app) const = 0;
    Q_INVOKABLE virtual QString errorMessage() const;
    Q_INVOKABLE virtual bool isResourceSupported(AbstractResource *res) const = 0;
    virtual bool isFetching() const = 0;
    virtual bool isReviewable() const;

public Q_SLOTS:
    virtual void login() = 0;
    virtual void registerAndLogin() = 0;
    virtual void logout() = 0;
    virtual void submitUsefulness(Review *r, bool useful) = 0;
    virtual void submitReview(AbstractResource *app, const QString &summary, const QString &review_text, const QString &rating) = 0;
    virtual void deleteReview(Review *r) = 0;
    virtual void flagReview(Review *r, const QString &reason, const QString &text) = 0;
    virtual void fetchReviews(AbstractResource *app, int page = 1) = 0;

Q_SIGNALS:
    void reviewsReady(AbstractResource *app, const QVector<ReviewPtr> &reviews, bool canFetchMore);
    void loginStateChanged();
    void error(const QString &message);
};

#endif // ABSTRACTREVIEWSBACKEND_H
