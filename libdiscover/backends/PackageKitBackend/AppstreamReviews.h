/***************************************************************************
 *   Copyright © 2016 Aleix Pol Gonzalez <aleixpol@blue-systems.com>       *
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

#ifndef APPSTREAMREVIEWS_H
#define APPSTREAMREVIEWS_H

#include <QHash>
#include <ReviewsBackend/AbstractReviewsBackend.h>

class KJob;
class AbstractResourcesBackend;

class AppstreamReviews : public AbstractReviewsBackend
{
    Q_OBJECT
public:
    AppstreamReviews(AbstractResourcesBackend* parent);

    Rating *ratingForApplication(AbstractResource *app) const override;

    QString userName() const override{ return {}; }
    bool hasCredentials() const override { return false; }
    void login() override {}
    void registerAndLogin() override {}
    void logout() override {}
    void submitUsefulness(Review* /*r*/, bool /*useful*/) override {}
    void submitReview(AbstractResource* /*app*/, const QString& /*summary*/, const QString& /*review_text*/, const QString& /*rating*/) override {}
    void deleteReview(Review* /*r*/) override {}

    void flagReview(Review* /*r*/, const QString& /*reason*/, const QString &/*text*/) override {}
    void fetchReviews(AbstractResource* /*app*/, int /*page*/) override {}
    bool isReviewable() const override { return false; }
    bool isFetching() const override;

private:
    void ratingsFetched(KJob* job);

    QHash<QString, Rating *> m_ratings;
    bool m_fetching;
};

#endif // APPSTREAMREVIEWS_H
