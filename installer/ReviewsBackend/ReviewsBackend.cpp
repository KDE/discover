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

#include <QtCore/QProcess>
#include <QtCore/QStringBuilder>

#include <KGlobal>
#include <KIO/Job>
#include <KLocale>
#include <KTemporaryFile>
#include <KUrl>
#include <KDebug>

#include <qjson/parser.h>

#include "../Application.h"
#include "Rating.h"
#include "Review.h"

ReviewsBackend::ReviewsBackend(QObject *parent)
        : QObject(parent)
        , m_serverBase("http://reviews.staging.ubuntu.com/reviews/api/1.0/")
        , m_ratingsFile(0)
        , m_reviewsFile(0)
{
    fetchRatings();
}

ReviewsBackend::~ReviewsBackend()
{
    delete m_ratingsFile;
    qDeleteAll(m_ratings);
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
    connect(getJob, SIGNAL(result(KJob *)),
            this, SLOT(ratingsFetched(KJob *)));
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
        m_ratings << rating;
    }
}

Rating *ReviewsBackend::ratingForApplication(Application *app) const
{
    foreach (Rating *rating, m_ratings) {
        if (rating->packageName() != app->package()->latin1Name()) {
            continue;
        }

        if (rating->applicationName() == app->name()) {
            return rating;
        }
    }

    return 0;
}

void ReviewsBackend::fetchReviews(Application *app)
{
    // Check our cache before fetching from the 'net
    QString hashName = app->package()->latin1Name() + app->name();
    if (m_reviewsCache.contains(hashName)) {
        emit reviewsReady(app, m_reviewsCache.value(hashName));
        return;
    }

    QString lang = getLanguage();
    QString origin = app->package()->origin().toLower();

    QString program = QLatin1String("lsb_release -c -s");
    QProcess lsb_release;
    lsb_release.start(program);
    lsb_release.waitForFinished();
    QString distroSeries = lsb_release.readAllStandardOutput();
    distroSeries = distroSeries.trimmed();

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
    KUrl reviewsUrl(m_serverBase % lang % '/' % origin % '/' % distroSeries %
                    '/' % version % '/' % packageName % ';' % appName % '/' %
                    QLatin1Literal("page") % '/' % '1');

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
    connect(getJob, SIGNAL(result(KJob *)),
            this, SLOT(reviewsFetched(KJob *)));
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
        kDebug() << "Summary:" << review->summary();
        reviewsList << review;
    }

    Application *app = m_jobHash.value(job);
    m_jobHash.remove(job);

    m_reviewsCache[app->package()->latin1Name() + app->name()] = reviewsList;

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

