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
#include <QtCore/QLocale>
#include <QDebug>
#include <QJsonDocument>
#include <QTemporaryFile>
#include <QStandardPaths>
#include <QFileInfo>
#include <QDir>

#include <KIO/Job>
#include <KLocalizedString>
#include <KCompressionDevice>

#include <QApt/Backend>

#include <qoauth/src/interface.h>

#include <Application.h>
#include <ReviewsBackend/Rating.h>
#include <ReviewsBackend/Review.h>
#include <ReviewsBackend/AbstractLoginBackend.h>
#include "UbuntuLoginBackend.h"
#include <resources/AbstractResourcesBackend.h>
#include <MuonDataSources.h>

static QString getCodename(const QString& value)
{
    QString ret;
    QFile f(QStringLiteral("/etc/os-release"));
    if(f.open(QIODevice::ReadOnly|QIODevice::Text)) {
        QRegExp rx(QStringLiteral("%1=(.+)\n").arg(value));
        while(!f.atEnd()) {
            QString line = QString::fromLatin1(f.readLine());
            if(rx.exactMatch(line)) {
                ret = rx.cap(1);
                break;
            }
        }
    }
    return ret;
}

ReviewsBackend::ReviewsBackend(QObject *parent)
        : AbstractReviewsBackend(parent)
        , m_aptBackend(0)
        , m_serverBase(MuonDataSources::rnRSource())
{
    m_distId = getCodename(QStringLiteral("ID"));
    m_loginBackend = new UbuntuLoginBackend(this);
    connect(m_loginBackend, &AbstractLoginBackend::connectionStateChanged, this, &ReviewsBackend::loginStateChanged);
    connect(m_loginBackend, &AbstractLoginBackend::connectionStateChanged, this, &ReviewsBackend::refreshConsumerKeys);
    m_oauthInterface = new QOAuth::Interface(this);
    
    QMetaObject::invokeMethod(this, "fetchRatings", Qt::QueuedConnection);
}

ReviewsBackend::~ReviewsBackend()
{}

void ReviewsBackend::refreshConsumerKeys()
{
    if(m_loginBackend->hasCredentials()) {
        m_oauthInterface->setConsumerKey(m_loginBackend->consumerKey());
        m_oauthInterface->setConsumerSecret(m_loginBackend->consumerSecret());
        
        QList<QPair<QString, QVariantMap> >::const_iterator it, itEnd;
        for(it=m_pendingRequests.constBegin(), itEnd=m_pendingRequests.constEnd(); it!=itEnd; ++it) {
            postInformation(it->first, it->second);
        }
        m_pendingRequests.clear();
    }
}

void ReviewsBackend::setAptBackend(QApt::Backend *aptBackend)
{
    m_aptBackend = aptBackend;
}

// void ReviewsBackend::clearReviewCache()
// {
//     foreach (QList<Review *> reviewList, m_reviewsCache) {
//         qDeleteAll(reviewList);
//     }
// 
//     m_reviewsCache.clear();
// }

void ReviewsBackend::fetchRatings()
{
    QString ratingsCache = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + QStringLiteral("/libdiscover/ratings.txt");
    QFileInfo file(ratingsCache);
    QDir::temp().mkpath(file.dir().path());
    QUrl ratingsUrl(m_serverBase.toString()+QStringLiteral("review-stats/"));
    //default to popcon if not using ubuntu
    if(m_distId.toLower() == QLatin1String("ubuntu")){
        refreshConsumerKeys();
        // First, load our old ratings cache in case we don't have net connectivity
        loadRatingsFromFile();
        // Try to fetch the latest ratings from the internet
    } else {
        ratingsUrl = QUrl(QStringLiteral("http://popcon.debian.org/all-popcon-results.gz"));
    }
    KIO::FileCopyJob *getJob = KIO::file_copy(ratingsUrl, QUrl::fromLocalFile(ratingsCache), -1,
                               KIO::Overwrite | KIO::HideProgressInfo);
    connect(getJob, &KIO::FileCopyJob::result, this, &ReviewsBackend::ratingsFetched);
}

void ReviewsBackend::ratingsFetched(KJob *job)
{
    if (job->error()) {
        qWarning() << "Couldn't fetch the ratings" <<  job->errorString();
        return;
    }

    loadRatingsFromFile();
}

void ReviewsBackend::loadRatingsFromFile()
{
    QString ratingsCache = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation)+QStringLiteral("/libdiscover/ratings.txt");
    QScopedPointer<QIODevice> dev(new KCompressionDevice(ratingsCache, KCompressionDevice::GZip));
    if (!dev->open(QIODevice::ReadOnly)) {
        qWarning() << "Couldn't open ratings.txt" << ratingsCache;
        return;
    }
    if(m_distId.toLower() == QLatin1String("ubuntu")) {
        QJsonParseError error;
        QJsonDocument doc = QJsonDocument::fromJson(dev->readAll(), &error);

        if (error.error != QJsonParseError::NoError) {
            qDebug() << "error while parsing ratings: " << ratingsCache;
            return;
        }

        QVariant ratings = doc.toVariant();
        qDeleteAll(m_ratings);
        m_ratings.clear();
        foreach (const QVariant &data, ratings.toList()) {
            Rating *rating = new Rating(data.toMap());
            if (!rating->ratingCount()) {
                delete rating;
                continue;
            }
            rating->setParent(this);
            m_ratings[rating->packageName()] = rating;
        }
    } else {
        if(dev->open(QIODevice::ReadOnly)) {
            while(!dev->atEnd()) {
                QString line = QString::fromLatin1(dev->readLine());
                QStringList lineContent = line.split(QLatin1Char(' '));
                if(lineContent.first() != QLatin1String("Package:") || lineContent.isEmpty()) {
                    continue;
                }
                QString pkgName = lineContent.at(1);
                lineContent.removeFirst();
                lineContent.removeFirst();

                Rating *rating = new Rating(pkgName,lineContent);
                if (!rating->ratingCount()) {
                    delete rating;
                    continue;
                }
                rating->setParent(this);
                m_ratings[rating->packageName()] = rating;
            }
        }
    }
    emit ratingsReady();
}

Rating *ReviewsBackend::ratingForApplication(AbstractResource* app) const
{
    return m_ratings.value(app->packageName());
}

void ReviewsBackend::stopPendingJobs()
{
    for(auto it = m_jobHash.constBegin(); it != m_jobHash.constEnd(); ++it) {
        disconnect(it.key(), SIGNAL(result(KJob*)), this, SLOT(changelogFetched(KJob*)));
    }
    m_jobHash.clear();
}

void ReviewsBackend::fetchReviews(AbstractResource* res, int page)
{
    Q_ASSERT(!res->backend()->isFetching());
    Application* app = qobject_cast<Application*>(res);
    // Check our cache before fetching from the 'net
    QString hashName = app->package()->name() + app->untranslatedName();
    
    QList<Review*> revs = m_reviewsCache.value(hashName);
    if (revs.size()>(page*10)) { //there are 10 reviews per page
        emit reviewsReady(app, revs.mid(page*10, 10));
        return;
    }

    QString lang = getLanguage();
    QString origin = app->package()->origin().toLower();

    QString version = QLatin1String("any");
    QString packageName = app->package()->name();
    QString appName = app->name();
    // Replace spaces with %2B for the url
    appName.replace(QLatin1Char(' '), QLatin1String("%2B"));

    // Figuring out how this damn Django url was put together took more
    // time than figuring out QJson...
    // But that could be because the Ubuntu Software Center (which I used to
    // figure it out) is written in python, so you have to go hunting to where
    // a variable was initially initialized with a primitive to figure out its type.
    QUrl reviewsUrl(m_serverBase.toString() + QLatin1String("/reviews/filter/") % lang % QLatin1Char('/')
            % origin % QLatin1Char('/') % QLatin1String("any") % QLatin1Char('/') % version % QLatin1Char('/') % packageName
            % QLatin1Char(';') % appName % QLatin1String("/page/") % QString::number(page));

    KIO::StoredTransferJob* getJob = KIO::storedGet(reviewsUrl, KIO::NoReload, KIO::Overwrite | KIO::HideProgressInfo);
    m_jobHash[getJob] = app;
    connect(getJob, &KIO::StoredTransferJob::result, this, &ReviewsBackend::reviewsFetched);
}

static Review* constructReview(const QVariantMap& data)
{
    QString reviewUsername = data.value(QStringLiteral("reviewer_username")).toString();
    QString reviewDisplayName = data.value(QStringLiteral("reviewer_displayname")).toString();
    QString reviewer = reviewDisplayName.isEmpty() ? reviewUsername : reviewDisplayName;
    return new Review(
        data.value(QStringLiteral("app_name")).toString(),
        data.value(QStringLiteral("package_name")).toString(),
        data.value(QStringLiteral("language")).toString(),
        data.value(QStringLiteral("summary")).toString(),
        data.value(QStringLiteral("review_text")).toString(),
        reviewer,
        QDateTime::fromString(data.value(QStringLiteral("date_created")).toString(), QStringLiteral("yyyy-MM-dd HH:mm:ss")),
        !data.value(QStringLiteral("hide")).toBool(),
        data.value(QStringLiteral("id")).toULongLong(),
        data.value(QStringLiteral("rating")).toInt() * 2,
        data.value(QStringLiteral("usefulness_total")).toInt(),
        data.value(QStringLiteral("usefulness_favorable")).toInt(),
        data.value(QStringLiteral("version")).toString());
}

void ReviewsBackend::reviewsFetched(KJob *j)
{
    KIO::StoredTransferJob* job = qobject_cast<KIO::StoredTransferJob*>(j);
    Application *app = m_jobHash.take(job);
    if (job->error() || !app) {
        return;
    }

    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(job->data(), &error);

    if (error.error != QJsonParseError::NoError) {
        return;
    }
    QVariant reviews = doc.toVariant();

    QList<Review *> reviewsList;
    foreach (const QVariant &data, reviews.toList()) {
        reviewsList << constructReview(data.toMap());
    }

    m_reviewsCache[app->package()->name() + app->name()].append(reviewsList);

    emit reviewsReady(app, reviewsList);
}

QString ReviewsBackend::getLanguage()
{
    // The reviews API abbreviates all langs past the _ char except these
    const QStringList fullLangs = { QStringLiteral("pt_BR"), QStringLiteral("zh_CN"), QStringLiteral("zh_TW") };

    QString language = QLocale().bcp47Name();

    if (fullLangs.contains(language)) {
        return language;
    }

    return language.split(QLatin1Char('_')).first();
}

void ReviewsBackend::submitUsefulness(Review* r, bool useful)
{
    QVariantMap data = { { QStringLiteral("useful"), useful } };
    
    postInformation(QStringLiteral("reviews/%1/recommendations/").arg(r->id()), data);
}

void ReviewsBackend::submitReview(AbstractResource* application, const QString& summary,
                      const QString& review_text, const QString& rating)
{
    Application* app = qobject_cast<Application*>(application);
    
    QVariantMap data = {
        { QStringLiteral("app_name"), app->name() },
        { QStringLiteral("package_name"), app->packageName() },
        { QStringLiteral("summary"), summary },
        { QStringLiteral("version"), app->package()->version() },
        { QStringLiteral("review_text"), review_text },
        { QStringLiteral("rating"), rating },
        { QStringLiteral("language"), getLanguage() },
        { QStringLiteral("origin"), app->package()->origin() }
    };

    QString distroSeries = getCodename(QStringLiteral("VERSION"));
    if(!distroSeries.isEmpty()){
        data[QStringLiteral("distroseries")] = distroSeries.split(QLatin1Char(' ')).last().remove(QLatin1Char('(')).remove(QLatin1Char(')'));
    }else{
        data[QStringLiteral("distroseries")] = getCodename(QStringLiteral("PRETTY_NAME")).split(QLatin1Char(' ')).last();
    }
    data[QStringLiteral("arch_tag")] = app->package()->architecture();
    
    postInformation(QStringLiteral("reviews/"), data);
}

void ReviewsBackend::deleteReview(Review* r)
{
    postInformation(QStringLiteral("reviews/delete/%1/").arg(r->id()), QVariantMap());
}

void ReviewsBackend::flagReview(Review* r, const QString& reason, const QString& text)
{
    QVariantMap data = {
        { QStringLiteral("reason"), reason },
        { QStringLiteral("text"), text }
    };

    postInformation(QStringLiteral("reviews/%1/flags/").arg(r->id()), data);
}

QByteArray authorization(QOAuth::Interface* oauth, const QUrl& url, AbstractLoginBackend* login)
{
    return oauth->createParametersString(url.url(), QOAuth::POST, login->token(), login->tokenSecret(),
                                           QOAuth::HMAC_SHA1, QOAuth::ParamMap(), QOAuth::ParseForHeaderArguments);
}

void ReviewsBackend::postInformation(const QString& path, const QVariantMap& data)
{
    if(!hasCredentials()) {
        m_pendingRequests += qMakePair(path, data);
        login();
        return;
    }
    
    QUrl url(m_serverBase.toString() +QLatin1Char('/')+ path);
    url.setScheme(QStringLiteral("https"));
    
    KIO::StoredTransferJob* job = KIO::storedHttpPost(QJsonDocument::fromVariant(data).toJson(), url, KIO::Overwrite | KIO::HideProgressInfo); //TODO port to QJsonDocument
    job->addMetaData(QStringLiteral("content-type"), QStringLiteral("Content-Type: application/json") );
    job->addMetaData(QStringLiteral("customHTTPHeader"), QStringLiteral("Authorization: ") + QString::fromLatin1(authorization(m_oauthInterface, url, m_loginBackend)));
    connect(job, &KIO::StoredTransferJob::result, this, &ReviewsBackend::informationPosted);
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

QString ReviewsBackend::errorMessage() const
{
    return i18n("No reviews available for Debian.");
}

bool ReviewsBackend::isReviewable() const
{
    QString m_distId = getCodename(QLatin1String("ID"));
    if(m_distId == QLatin1String("ubuntu")){
        return true;
    }
    return false;
}

