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

#include "KNSReviews.h"
#include "KNSBackend.h"
#include "KNSResource.h"
#include <ReviewsBackend/Rating.h>
#include <resources/AbstractResource.h>
#include <attica/provider.h>
#include <attica/providermanager.h>
#include <attica/content.h>

KNSReviews::KNSReviews(KNSBackend* backend, QObject* parent)
    : AbstractReviewsBackend(parent)
    , m_backend(backend)
    , m_fetching(0)
{
    if(!m_backend->isFetching())
        connect(m_backend, SIGNAL(backendReady()), SIGNAL(ratingsReady()));
    else
        QMetaObject::invokeMethod(this, "ratingsReady", Qt::QueuedConnection);
}

Rating* KNSReviews::ratingForApplication(AbstractResource* app) const
{
    Attica::Content c = qobject_cast<KNSResource*>(m_backend->resourceByPackageName(app->packageName()))->content();
    QVariantMap data;
    data["package_name"] = app->packageName();
    data["app_name"] = app->name();
    data["ratings_total"] = c.numberOfComments();
    data["ratings_average"] = c.rating();
    data["histogram"] = "";
    return new Rating(data);
}

void KNSReviews::fetchReviews(AbstractResource* app, int page)
{
    emit reviewsReady(app, QList<Review*>());
}

bool KNSReviews::isFetching() const
{
    return m_backend->isFetching();
}

void KNSReviews::flagReview(Review* r, const QString& reason, const QString& text)
{

}

void KNSReviews::deleteReview(Review* r)
{

}

void KNSReviews::submitReview(AbstractResource* app, const QString& summary, const QString& review_text, const QString& rating)
{

}

void KNSReviews::submitUsefulness(Review* r, bool useful)
{

}

void KNSReviews::logout()
{

}

void KNSReviews::registerAndLogin()
{

}

void KNSReviews::login()
{

}

bool KNSReviews::hasCredentials() const
{
    return false;
}

QString KNSReviews::userName() const
{
    return "little joe";
}
