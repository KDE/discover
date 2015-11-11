/***************************************************************************
 *   Copyright © 2012 Aleix Pol Gonzalez <aleixpol@blue-systems.com>       *
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

#ifndef KNSREVIEWS_H
#define KNSREVIEWS_H

#include <ReviewsBackend/AbstractReviewsBackend.h>
#include <QMap>
#include <attica/content.h>

class KNSBackend;
class QUrl;
namespace Attica {
class Provider;
class ProviderManager;
class BaseJob;
}

class KNSReviews : public AbstractReviewsBackend
{
    Q_OBJECT
    public:
        explicit KNSReviews(KNSBackend* backend);

        virtual void fetchReviews(AbstractResource* app, int page = 1) override;
        virtual bool isFetching() const override;
        virtual void flagReview(Review* r, const QString& reason, const QString& text) override;
        virtual void deleteReview(Review* r) override;
        virtual void submitReview(AbstractResource* app, const QString& summary, const QString& review_text, const QString& rating) override;
        virtual void submitUsefulness(Review* r, bool useful) override;
        virtual void logout() override;
        virtual void registerAndLogin() override;
        virtual void login() override;
        virtual Rating* ratingForApplication(AbstractResource* app) const override;
        virtual bool hasCredentials() const override;
        virtual QString userName() const override;

    private Q_SLOTS:
        void commentsReceived(Attica::BaseJob* job);
        void credentialsReceived(const QString& user, const QString& password);

    private:
        KNSBackend* m_backend;

        int m_fetching;
};

#endif // KNSREVIEWS_H
