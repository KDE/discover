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
#include <ReviewsBackend/Review.h>
#include <resources/AbstractResource.h>
#include <attica/provider.h>
#include <attica/providermanager.h>
#include <attica/content.h>
#include <QDebug>

Q_DECLARE_METATYPE(AbstractResource*)

KNSReviews::KNSReviews(KNSBackend* backend)
    : AbstractReviewsBackend(backend)
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
    KNSResource *resource = qobject_cast<KNSResource*>(m_backend->resourceByPackageName(app->packageName()));
    if (!resource)
    {
        qDebug() << app->packageName() << "<= couldn't find resource";
        return nullptr;
    }

    Attica::Content c = resource->content();
    Q_ASSERT(c.rating()<=100);
    QVariantMap data;
    data["package_name"] = app->packageName();
    data["app_name"] = app->name();
    data["ratings_total"] = c.numberOfComments();
    data["ratings_average"] = c.rating()/20;
    data["histogram"] = "";
    return new Rating(data);
}

void KNSReviews::fetchReviews(AbstractResource* app, int page)
{
    if(!m_backend->provider()->hasCommentService()) {
        emit reviewsReady(app, QList<Review*>());
        return;
    }
    
    Attica::ListJob< Attica::Comment >* job =
        m_backend->provider()->requestComments(Attica::Comment::ContentComment, app->packageName(), "0", page, 10);
    job->setProperty("app", qVariantFromValue<AbstractResource*>(app));
    connect(job, SIGNAL(finished(Attica::BaseJob*)), SLOT(commentsReceived(Attica::BaseJob*)));
    job->start();
}

void KNSReviews::commentsReceived(Attica::BaseJob* j)
{
    Attica::ListJob<Attica::Comment>* job = static_cast<Attica::ListJob<Attica::Comment>*>(j);
    Attica::Comment::List comments = job->itemList();
    
    QList<Review*> reviews;
    AbstractResource* app = job->property("app").value<AbstractResource*>();
    foreach(const Attica::Comment& comment, comments) {
        //TODO: language lookup?
        Review* r = new Review(app->name(), app->packageName(), "en", comment.subject(), comment.text(), comment.user(),
            comment.date(), true, comment.id().toInt(), comment.score()/10, 0, 0, QString()
        );
        reviews += r;
    }
    
    emit reviewsReady(app, reviews);
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
    return m_backend->provider()->hasCredentials();
}

QString KNSReviews::userName() const
{
    return "little joe";
}
