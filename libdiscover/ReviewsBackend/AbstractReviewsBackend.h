/***************************************************************************
 *   Copyright © 2012 Aleix Pol Gonzalez <aleixpol@blue-systems.com        *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU General Public License as        *
 *   published by the Free Software Foundation; either version 2 of        *
 *   the License or (at your option) version 3 or any later version        *
 *   accepted by the membership of KDE e.V. (or its successor approved     *
 *   by the membership of KDE e.V.), which shall act as a proxy            *
 *   defined in Section 14 of version 3 of the license.                    *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>. *
 ***************************************************************************/

#ifndef ABSTRACTREVIEWSBACKEND_H
#define ABSTRACTREVIEWSBACKEND_H

#include <QObject>

#include "discovercommon_export.h"

class Rating;
class AbstractResource;
class Review;

class DISCOVERCOMMON_EXPORT AbstractReviewsBackend : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool isReviewable READ isReviewable CONSTANT)
    Q_PROPERTY(bool hasCredentials READ hasCredentials NOTIFY loginStateChanged)
    Q_PROPERTY(QString name READ userName NOTIFY loginStateChanged)
    public:
        explicit AbstractReviewsBackend(QObject* parent = nullptr);

        virtual QString userName() const = 0;
        virtual bool hasCredentials() const = 0;

        Q_SCRIPTABLE virtual Rating *ratingForApplication(AbstractResource *app) const = 0;
        Q_INVOKABLE virtual QString errorMessage() const;
    public slots:
        virtual void login() = 0;
        virtual void registerAndLogin() = 0;
        virtual void logout() = 0;
        virtual void submitUsefulness(Review* r, bool useful) = 0;
        virtual void submitReview(AbstractResource* app, const QString& summary,
                        const QString& review_text, const QString& rating) = 0;
        virtual void deleteReview(Review* r) = 0;
        virtual void flagReview(Review* r, const QString& reason, const QString &text) = 0;
        virtual bool isFetching() const = 0;
        virtual void fetchReviews(AbstractResource* app, int page=1) = 0;
        virtual bool isReviewable() const;

    Q_SIGNALS:
        void reviewsReady(AbstractResource *app, QList<Review *>);
        void ratingsReady();
        void loginStateChanged();
};

#endif // ABSTRACTREVIEWSBACKEND_H
