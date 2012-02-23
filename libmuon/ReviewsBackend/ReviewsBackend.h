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

#include "libmuonprivate_export.h"

class KJob;
class KTemporaryFile;

namespace QApt {
    class Backend;
}

class AbstractLoginBackend;
class Application;
class Rating;
class Review;

class MUONPRIVATE_EXPORT ReviewsBackend : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool hasCredentials READ hasCredentials NOTIFY loginStateChanged)
    Q_PROPERTY(QString name READ userName NOTIFY loginStateChanged)
public:
    ReviewsBackend(QObject *parent);
    ~ReviewsBackend();

    Q_SCRIPTABLE Rating *ratingForApplication(Application *app) const;

    void setAptBackend(QApt::Backend *aptBackend);
    void fetchReviews(Application* app, int page=1);
    void clearReviewCache();
    void stopPendingJobs();
    bool isFetching() const;

    QString userName() const;
    bool hasCredentials() const;

signals:
    void loginStateChanged();

private:
    QApt::Backend *m_aptBackend;

    QString m_serverBase;
    KTemporaryFile *m_ratingsFile;
    KTemporaryFile *m_reviewsFile;
    QHash<QString, Rating *> m_ratings;
    // cache key is package name + app name, since both by their own may not be unique
    QHash<QString, QList<Review *> > m_reviewsCache;
    QHash<KJob *, Application *> m_jobHash;

    void fetchRatings();
    QString getLanguage();
    AbstractLoginBackend* m_loginBackend;

private Q_SLOTS:
    void ratingsFetched(KJob *job);
    void reviewsFetched(KJob *job);

public slots:
    void login();
    void registerAndLogin();
    void logout();

Q_SIGNALS:
    void reviewsReady(Application *app, QList<Review *>);
    void ratingsReady();
};

#endif
