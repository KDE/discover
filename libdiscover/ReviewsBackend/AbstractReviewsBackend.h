/*
 *   SPDX-FileCopyrightText: 2012 Aleix Pol Gonzalez <aleixpol@blue-systems.com
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include <QObject>

#include "Review.h"
#include "ReviewsModel.h"
class Rating;
class AbstractResource;

class DISCOVERCOMMON_EXPORT AbstractReviewsBackend : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool isReviewable READ isReviewable CONSTANT)
    Q_PROPERTY(bool supportsNameChange READ supportsNameChange CONSTANT)
    Q_PROPERTY(bool hasCredentials READ hasCredentials)
    Q_PROPERTY(QString preferredUserName READ preferredUserName NOTIFY preferredUserNameChanged)
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY errorMessageChanged)
public:
    explicit AbstractReviewsBackend(QObject *parent = nullptr);

    virtual bool hasCredentials() const = 0;

    Q_SCRIPTABLE virtual Rating *ratingForApplication(AbstractResource *app) const = 0;
    Q_INVOKABLE virtual QString errorMessage() const;
    Q_INVOKABLE virtual bool isResourceSupported(AbstractResource *res) const = 0;
    virtual bool isFetching() const = 0;
    virtual bool isReviewable() const;
    virtual bool supportsNameChange() const;

public Q_SLOTS:
    virtual void login() = 0;
    virtual void registerAndLogin() = 0;
    virtual void logout() = 0;
    virtual void submitUsefulness(Review *r, bool useful) = 0;
    // About all the different "user_names": the user_name that is taken as input here
    // is the user_name the user typed in the review dialog, which defaults to what
    // the backend returns in userName() or the last username used
    // if the backend supports changing it (that is what the preferredUserName is).
    // If the backend returns true for "supportsNameChange()", then the review dialog won't let them change it,
    // therefore making the user_name here the same as "userName()".
    void submitReview(AbstractResource *app, const QString &summary, const QString &review_text, const QString &rating, const QString &userName);
    QString preferredUserName() const;
    virtual void deleteReview(Review *r) = 0;
    virtual void flagReview(Review *r, const QString &reason, const QString &text) = 0;
    virtual void fetchReviews(AbstractResource *app, int page = 1) = 0;

Q_SIGNALS:
    void reviewsReady(AbstractResource *app, const QVector<ReviewPtr> &reviews, bool canFetchMore);
    void error(const QString &message);
    void fetchingChanged(bool fetching);
    void preferredUserNameChanged();
    void errorMessageChanged();

protected:
    virtual void sendReview(AbstractResource *app, const QString &summary, const QString &review_text, const QString &rating, const QString &userName) = 0;
    virtual QString userName() const = 0;
};
