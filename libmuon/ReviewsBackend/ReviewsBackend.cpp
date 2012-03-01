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

#include "ReviewsBackend.h"

#include <QtCore/QStringBuilder>
#include <QDebug>

#include <KGlobal>
#include <KIO/Job>
#include <KLocale>
#include <KTemporaryFile>
#include <KUrl>

#include <LibQApt/Backend>

#include <qjson/parser.h>
#include <qjson/serializer.h>

#include <QtOAuth/interface.h>

#include "../Application.h"
#include "Rating.h"
#include "Review.h"
#include "AbstractLoginBackend.h"
#include "UbuntuLoginBackend.h"

ReviewsBackend::ReviewsBackend(QObject *parent)
        : QObject(parent)
        , m_aptBackend(0)
        , m_serverBase("http://reviews.ubuntu.com/reviews/api/1.0/")
        , m_ratingsFile(0)
        , m_reviewsFile(0)
{
    m_loginBackend = new UbuntuLoginBackend(this);
    connect(m_loginBackend, SIGNAL(connectionStateChanged()), SIGNAL(loginStateChanged()));
    fetchRatings();
    m_oauthInterface = new QOAuth::Interface(this);
    m_oauthInterface->setConsumerKey(m_loginBackend->consumerKey());
    m_oauthInterface->setConsumerSecret(m_loginBackend->consumerSecret());
}

ReviewsBackend::~ReviewsBackend()
{
    delete m_ratingsFile;
    qDeleteAll(m_ratings);
}

void ReviewsBackend::setAptBackend(QApt::Backend *aptBackend)
{
    m_aptBackend = aptBackend;
}

void ReviewsBackend::clearReviewCache()
{
    foreach (QList<Review *> reviewList, m_reviewsCache) {
        qDeleteAll(reviewList);
    }

    m_reviewsCache.clear();
}

void ReviewsBackend::fetchRatings()
{
    KUrl ratingsUrl(m_serverBase % "review-stats/");

    if (m_ratingsFile) {
        m_ratingsFile->deleteLater();
        m_ratingsFile = 0;
    }

    m_ratingsFile = new KTemporaryFile();
    m_ratingsFile->open();

    KIO::FileCopyJob *getJob = KIO::file_copy(ratingsUrl,
                               m_ratingsFile->fileName(), -1,
                               KIO::Overwrite | KIO::HideProgressInfo);
    connect(getJob, SIGNAL(result(KJob*)),
            this, SLOT(ratingsFetched(KJob*)));
}

void ReviewsBackend::ratingsFetched(KJob *job)
{
    if (job->error()) {
        return;
    }

    QFile file(m_ratingsFile->fileName());
    file.open(QIODevice::ReadOnly | QIODevice::Text);

    QJson::Parser parser;
    QByteArray json = file.readAll();

    bool ok = false;
    QVariant ratings = parser.parse(json, &ok);

    if (!ok) {
        return;
    }

    qDeleteAll(m_ratings);
    m_ratings.clear();
    foreach (const QVariant &data, ratings.toList()) {
        Rating *rating = new Rating(data.toMap());
        if (!rating->ratingCount()) {
            delete rating;
            continue;
        }
        m_ratings[rating->packageName()] = rating;
    }
    emit ratingsReady();
}

Rating *ReviewsBackend::ratingForApplication(Application *app) const
{
    return m_ratings.value(app->packageName());
}

void ReviewsBackend::stopPendingJobs()
{
    auto iter = m_jobHash.constBegin();
    while (iter != m_jobHash.constEnd()) {
        KJob *getJob = iter.key();
        disconnect(getJob, SIGNAL(result(KJob*)),
                   this, SLOT(changelogFetched(KJob*)));
        iter++;
    }

    m_jobHash.clear();
}

void ReviewsBackend::fetchReviews(Application *app, int page)
{
    // Check our cache before fetching from the 'net
    QString hashName = app->package()->latin1Name() + app->untranslatedName();
    
    QList<Review*> revs = m_reviewsCache.value(hashName);
    if (revs.size()>(page*5)) { //there are 5 reviews per page
        emit reviewsReady(app, revs.mid(page*5, 5));
        return;
    }

    QString lang = getLanguage();
    QString origin = app->package()->origin().toLower();

    QString version = QLatin1String("any");
    QString packageName = app->package()->latin1Name();
    QString appName = app->name();
    // Replace spaces with %2B for the url
    appName.replace(' ', QLatin1String("%2B"));

    // Figuring out how this damn Django url was put together took more
    // time than figuring out QJson...
    // But that could be because the Ubuntu Software Center (which I used to
    // figure it out) is written in python, so you have to go hunting to where
    // a variable was initially initialized with a primitive to figure out its type.
    KUrl reviewsUrl(m_serverBase % QLatin1String("reviews/filter/") % lang % '/'
		    % origin % '/' % QLatin1String("any") % '/' % version % '/' % packageName
		    % ';' % appName % '/' % QLatin1String("page") % '/' % QString::number(page));

    if (m_reviewsFile) {
        m_reviewsFile->deleteLater();
        m_reviewsFile = 0;
    }

    m_reviewsFile = new KTemporaryFile();
    m_reviewsFile->open();

    KIO::FileCopyJob *getJob = KIO::file_copy(reviewsUrl,
                               m_reviewsFile->fileName(), -1,
                               KIO::Overwrite | KIO::HideProgressInfo);
    m_jobHash[getJob] = app;
    connect(getJob, SIGNAL(result(KJob*)),
            this, SLOT(reviewsFetched(KJob*)));
}

void ReviewsBackend::reviewsFetched(KJob *job)
{
    if (job->error()) {
        return;
    }

    QFile file(m_reviewsFile->fileName());
    file.open(QIODevice::ReadOnly | QIODevice::Text);

    QJson::Parser parser;
    QByteArray json = file.readAll();

    bool ok = false;
    QVariant reviews = parser.parse(json, &ok);

    if (!ok) {
        return;
    }

    QList<Review *> reviewsList;
    foreach (const QVariant &data, reviews.toList()) {
        Review *review = new Review(data.toMap());
        QApt::Package *package = m_aptBackend->package(review->packageName());

        if (package) {
            review->setPackage(package);
            reviewsList << review;
        }
    }

    Application *app = m_jobHash.value(job);
    m_jobHash.remove(job);

    m_reviewsCache[app->package()->latin1Name() + app->name()].append(reviewsList);

    emit reviewsReady(app, reviewsList);
}

QString ReviewsBackend::getLanguage()
{
    QStringList fullLangs;
    // The reviews API abbreviates all langs past the _ char except these
    fullLangs << "pt_BR" << "zh_CN" << "zh_TW";

    QString language = KGlobal::locale()->language();

    if (fullLangs.contains(language)) {
        return language;
    }

    return language.split('_').first();
}

void ReviewsBackend::submitUsefulness(Review* r, bool useful)
{
    QVariantMap data;
    data["useful"] = useful;
    
    postInformation(QString("/reviews/%1/recommendations/").arg(r->id()), data);
}

QByteArray authorization(QOAuth::Interface* oauth, const KUrl& url, AbstractLoginBackend* login)
{
    return oauth->createParametersString(url.url(), QOAuth::POST, login->token(), login->tokenSecret(),
                                           QOAuth::HMAC_SHA1, QOAuth::ParamMap(), QOAuth::ParseForHeaderArguments);
}

void ReviewsBackend::postInformation(const QString& path, const QVariantMap& data)
{
    KUrl url(m_serverBase);
    url.setScheme("https");
    url.addPath(path);
    
    KIO::StoredTransferJob* job = KIO::storedHttpPost(QJson::Serializer().serialize(data), url, KIO::Overwrite | KIO::HideProgressInfo);
    job->addMetaData("content-type", "Content-Type: application/json" );
    job->addMetaData("customHTTPHeader", "Authorization: " + authorization(m_oauthInterface, url, m_loginBackend));
    connect(job, SIGNAL(result(KJob*)), this, SLOT(informationPosted(KJob*)));
    job->start();
}

void ReviewsBackend::informationPosted(KJob* j)
{
    KIO::StoredTransferJob* job = qobject_cast<KIO::StoredTransferJob*>(j);
    if(job->error()==0) {
        qDebug() << "success" << job->data();
    } else {
        qDebug() << "error..." << job->error() << job->errorString() << job->errorText();
    }
}

bool ReviewsBackend::isFetching() const
{
    return !m_jobHash.isEmpty();
}

bool ReviewsBackend::hasCredentials() const
{
    return m_loginBackend->hasCredentials();
}

QString ReviewsBackend::userName() const
{
    Q_ASSERT(m_loginBackend->hasCredentials());
    return m_loginBackend->displayName();
}

void ReviewsBackend::login()
{
    Q_ASSERT(!m_loginBackend->hasCredentials());
    m_loginBackend->login();
}

void ReviewsBackend::registerAndLogin()
{
    Q_ASSERT(!m_loginBackend->hasCredentials());
    m_loginBackend->registerAndLogin();
}

void ReviewsBackend::logout()
{
    Q_ASSERT(m_loginBackend->hasCredentials());
    m_loginBackend->logout();
}
