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
#include <attica/provider.h>

class KNSBackend;
class QUrl;
namespace Attica {
class ProviderManager;
class BaseJob;
}

class KNSReviews : public AbstractReviewsBackend
{
    Q_OBJECT
    public:
        explicit KNSReviews(KNSBackend* backend);

        void fetchReviews(AbstractResource* app, int page = 1) override;
        bool isFetching() const override;
        void flagReview(Review* r, const QString& reason, const QString& text) override;
        void deleteReview(Review* r) override;
        void submitReview(AbstractResource* app, const QString& summary, const QString& review_text, const QString& rating) override;
        void submitUsefulness(Review* r, bool useful) override;
        void logout() override;
        void registerAndLogin() override;
        void login() override;
        Rating* ratingForApplication(AbstractResource* app) const override;
        bool hasCredentials() const override;
        QString userName() const override;

        void setProviderUrl(const QUrl &url);

    private Q_SLOTS:
        void commentsReceived(Attica::BaseJob* job);
        void credentialsReceived(const QString& user, const QString& password);

    private:
        Attica::Provider provider() const;
        KNSBackend* m_backend;
        QUrl m_providerUrl;
};

#endif // KNSREVIEWS_H
