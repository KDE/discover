/***************************************************************************
 *   Copyright © 2011 Jonathan Thomas <echidnaman@kubuntu.org>             *
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

#ifndef REVIEWSBACKEND_H
#define REVIEWSBACKEND_H

#include <QtCore/QString>
#include <QtCore/QVariant>
#include <KUrl>

#include "libmuonprivate_export.h"
#include <ReviewsBackend/AbstractReviewsBackend.h>

namespace QOAuth {
    class Interface;
}

class KJob;
class KTemporaryFile;

namespace QApt {
    class Backend;
}

class AbstractLoginBackend;
class Application;
class Rating;
class Review;

class MUONPRIVATE_EXPORT ReviewsBackend : public AbstractReviewsBackend
{
    Q_OBJECT
public:
    ReviewsBackend(QObject *parent);
    ~ReviewsBackend();

    Rating *ratingForApplication(AbstractResource *app) const;

    void setAptBackend(QApt::Backend *aptBackend);
    void fetchReviews(AbstractResource* app, int page=1);
//     void clearReviewCache();
    void stopPendingJobs();
    bool isFetching() const;

    QString userName() const;
    bool hasCredentials() const;

private:
    QApt::Backend *m_aptBackend;

    const KUrl m_serverBase;
    KTemporaryFile *m_ratingsFile;
    QHash<QString, Rating *> m_ratings;
    // cache key is package name + app name, since both by their own may not be unique
    QHash<QString, QList<Review *> > m_reviewsCache;
    QHash<KJob *, Application *> m_jobHash;

    void loadRatingsFromFile(const QString &fileName);
    QString getLanguage();
    AbstractLoginBackend* m_loginBackend;
    QOAuth::Interface* m_oauthInterface;
    QList<QPair<QString, QVariantMap> > m_pendingRequests;

private Q_SLOTS:
    void ratingsFetched(KJob *job);
    void reviewsFetched(KJob *job);
    void informationPosted(KJob* job);
    void postInformation(const QString& path, const QVariantMap& data);
    void fetchRatings();

public slots:
    void login();
    void registerAndLogin();
    void logout();
    void submitUsefulness(Review* r, bool useful);
    void submitReview(AbstractResource* app, const QString& summary,
                      const QString& review_text, const QString& rating);
    void deleteReview(Review* r);
    void flagReview(Review* r, const QString& reason, const QString &text);
    void refreshConsumerKeys();
};

#endif
