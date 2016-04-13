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
#include <KLocalizedString>
#include <KPasswordDialog>
#include <QDebug>
#include <QDesktopServices>

Q_DECLARE_METATYPE(AbstractResource*)

KNSReviews::KNSReviews(KNSBackend* backend)
    : AbstractReviewsBackend(backend)
    , m_backend(backend)
    , m_fetching(0)
{
    if(m_backend->isFetching())
        connect(m_backend, &KNSBackend::fetchingChanged, this, &KNSReviews::ratingsReady);
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
    QVariantMap data = {
        { QStringLiteral("package_name"), QVariant::fromValue(app->packageName()) },
        { QStringLiteral("app_name"), QVariant::fromValue(app->name()) },
        { QStringLiteral("ratings_total"), QVariant::fromValue(c.numberOfComments()) },
        { QStringLiteral("ratings_average"), QVariant::fromValue(c.rating()/20) },
        { QStringLiteral("histogram"), QVariant::fromValue<QString>(QLatin1Char('[')+QString::number(c.numberOfComments()*c.rating())+QLatin1Char(']')) }
    };
    return new Rating(data);
}

void KNSReviews::fetchReviews(AbstractResource* app, int page)
{
    Attica::ListJob< Attica::Comment >* job =
        m_backend->provider()->requestComments(Attica::Comment::ContentComment, app->packageName(), QStringLiteral("0"), page, 10);
    job->setProperty("app", qVariantFromValue<AbstractResource*>(app));
    connect(job, &Attica::BaseJob::finished, this, &KNSReviews::commentsReceived);
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
        Review* r = new Review(app->name(), app->packageName(), QStringLiteral("en"), comment.subject(), comment.text(), comment.user(),
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

void KNSReviews::flagReview(Review* , const QString& , const QString& )
{
    qWarning() << "cannot flag reviews";
}

void KNSReviews::deleteReview(Review* )
{
    qWarning() << "cannot delete comments";
}

void KNSReviews::submitReview(AbstractResource* app, const QString& summary, const QString& review_text, const QString& /*rating*/)
{
    m_backend->provider()->addNewComment(Attica::Comment::ContentComment, app->packageName(), QString(), QString(), summary, review_text);
}

void KNSReviews::submitUsefulness(Review* r, bool useful)
{
    m_backend->provider()->voteForComment(QString::number(r->id()), useful*5);
}

void KNSReviews::logout()
{
    bool b = m_backend->provider()->saveCredentials(QString(), QString());
    if (!b)
        qWarning() << "couldn't log out";
}

void KNSReviews::registerAndLogin()
{
    QDesktopServices::openUrl(m_backend->provider()->baseUrl());
}

void KNSReviews::login()
{
    KPasswordDialog* dialog = new KPasswordDialog;
    dialog->setPrompt(i18n("Log in information for %1", m_backend->provider()->name()));
    connect(dialog, &KPasswordDialog::gotUsernameAndPassword, this, &KNSReviews::credentialsReceived);
}

void KNSReviews::credentialsReceived(const QString& user, const QString& password)
{
    bool b = m_backend->provider()->saveCredentials(user, password);
    if (!b)
        qWarning() << "couldn't save" << user << "credentials for" << m_backend->provider()->name();
}

bool KNSReviews::hasCredentials() const
{
    return m_backend->provider()->hasCredentials();
}

QString KNSReviews::userName() const
{
    QString user, password;
    m_backend->provider()->loadCredentials(user, password);
    return user;
}
