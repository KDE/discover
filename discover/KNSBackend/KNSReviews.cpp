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
#include <ReviewsBackend/Rating.h>
#include <resources/AbstractResource.h>

//Big TODO: all this class
//Figure out how to do it? attica for the rescue?

KNSReviews::KNSReviews(QObject* parent)
    : AbstractReviewsBackend(parent)
{
}

void KNSReviews::fetchReviews(AbstractResource* app, int page)
{

}

bool KNSReviews::isFetching() const
{
    return false;
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

Rating* KNSReviews::ratingForApplication(AbstractResource* app) const
{
    QVariantMap data;
    data["package_name"] = app->packageName();
    data["app_name"] = app->name();
    data["ratings_total"] = 0;
    data["ratings_average"] = 0;
    data["histogram"] = "";
    return new Rating(data);
}

bool KNSReviews::hasCredentials() const
{
    return false;
}

QString KNSReviews::userName() const
{
    return "little joe";
}

