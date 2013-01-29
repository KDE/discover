/***************************************************************************
 *   Copyright © 2013 Aleix Pol Gonzalez <aleixpol@blue-systems.com>       *
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

#ifndef DUMMYREVIEWSBACKEND_H
#define DUMMYREVIEWSBACKEND_H

#include <ReviewsBackend/AbstractReviewsBackend.h>
#include <QMap>

class DummyBackend;
class DummyReviewsBackend : public AbstractReviewsBackend
{
Q_OBJECT
public:
    explicit DummyReviewsBackend(DummyBackend* parent = 0);

    virtual QString userName() const { return "dummy"; }
    virtual void login() {}
    virtual void logout() {}
    virtual void registerAndLogin() {}

    virtual Rating* ratingForApplication(AbstractResource* app) const;
    virtual bool hasCredentials() const { return false; }
    virtual void deleteReview(Review*) {}
    virtual void fetchReviews(AbstractResource* app, int page = 1);
    virtual bool isFetching() const { return false; }
    virtual void submitReview(AbstractResource*, const QString&, const QString&, const QString&) {}
    virtual void flagReview(Review*, const QString&, const QString&) {}
    virtual void submitUsefulness(Review*, bool) {}

private:
    QMap<AbstractResource*, Rating*> m_ratings;
};

#endif // DUMMYREVIEWSBACKEND_H
