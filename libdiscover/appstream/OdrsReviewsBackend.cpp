/***************************************************************************
 *   Copyright © 2013 Aleix Pol Gonzalez <aleixpol@blue-systems.com>       *
 *   Copyright © 2017 Jan Grulich <jgrulich@redhat.com>                    *
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

#include "OdrsReviewsBackend.h"

#include <ReviewsBackend/Review.h>
#include <ReviewsBackend/Rating.h>

#include <resources/AbstractResource.h>
#include <resources/AbstractResourcesBackend.h>

#include <KIO/FileCopyJob>
#include <KUser>

#include <QCryptographicHash>
#include <QDir>
#include <QDebug>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QFile>
#include <QFileInfo>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QStandardPaths>

#define APIURL "https://odrs.gnome.org/1.0/reviews/api"

OdrsReviewsBackend::OdrsReviewsBackend(AbstractResourcesBackend *parent)
    : AbstractReviewsBackend(parent)
    , m_isFetching(false)
    , m_nam(new QNetworkAccessManager(this))
{
    bool fetchRatings = false;
    const QUrl ratingsUrl(QStringLiteral(APIURL "/ratings"));
    const QUrl fileUrl = QUrl::fromLocalFile(QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + QStringLiteral("/ratings/ratings"));
    const QDir cacheDir(QStandardPaths::writableLocation(QStandardPaths::CacheLocation));

    // Create $HOME/.cache/discover/ratings folder
    cacheDir.mkdir(QStringLiteral("ratings"));

    if (QFileInfo::exists(fileUrl.toLocalFile())) {
        QFileInfo file(fileUrl.toLocalFile());
        // Refresh the cached ratings if they are older than one day
        if (file.lastModified().msecsTo(QDateTime::currentDateTime()) > 1000 * 60 * 60 * 24) {
            fetchRatings = true;
        }
    } else {
        fetchRatings = true;
    }

    if (fetchRatings) {
        m_isFetching = true;
        KIO::FileCopyJob *getJob = KIO::file_copy(ratingsUrl, fileUrl, -1, KIO::Overwrite | KIO::HideProgressInfo);
        connect(getJob, &KIO::FileCopyJob::result, this, &OdrsReviewsBackend::ratingsFetched);
    } else {
        parseRatings();
    }
}

void OdrsReviewsBackend::ratingsFetched(KJob *job)
{
    m_isFetching = false;
    if (job->error()) {
        qWarning() << "Failed to fetch ratings " << job->errorString();
    } else {
        parseRatings();
    }
}

static QString osName()
{
    QString osReleaseFilename;
    if (QFileInfo::exists(QStringLiteral("/etc/os-release"))) {
        osReleaseFilename = QStringLiteral("/etc/os-release");
    } else if (QFileInfo::exists(QStringLiteral("/usr/lib/os-release"))) {
        osReleaseFilename = QStringLiteral("/usr/lib/os-release");
    }

    if (osReleaseFilename.isEmpty()) {
        return QStringLiteral("Unknown");
    }

    QFile osReleaseFile(osReleaseFilename);
    if (osReleaseFile.open(QIODevice::ReadOnly)) {
        QString line;
        QTextStream stream(&osReleaseFile);
        while (stream.readLineInto(&line)) {
            if (line.startsWith(QStringLiteral("NAME"))) {
                QStringRef name = line.midRef(5).trimmed();
                if (name.startsWith(QLatin1Char('\"')) && name.endsWith(QLatin1Char('\"')))
                    name = name.mid(1, name.size()-2);
                return name.toString();
            }
        }
    }

    return QStringLiteral("Unknown");
}

static QString userHash()
{
    QString machineId;
    QFile file(QStringLiteral("/etc/machine-id"));
    if (file.open(QIODevice::ReadOnly)) {
        machineId = QString::fromUtf8(file.readAll());
        file.close();
    }

    if (machineId.isEmpty()) {
        return QString();
    }

    QString salted = QStringLiteral("gnome-software[%1:%2]").arg(KUser().loginName()).arg(machineId);
    return QString::fromUtf8(QCryptographicHash::hash(salted.toUtf8(), QCryptographicHash::Sha1).toHex());
}

void OdrsReviewsBackend::fetchReviews(AbstractResource *app, int page)
{
    Q_UNUSED(page)
    m_isFetching = true;

    // Check cached reviews
    const QString fileName = QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + QStringLiteral("/reviews/%1.json").arg(app->appstreamId());
    if (QFileInfo::exists(fileName)) {
        QFileInfo file(fileName);
        // Check if the reviews are not older than a week               msecs * secs * hours * days
        if (file.lastModified().msecsTo(QDateTime::currentDateTime()) < 1000 * 60 * 60 * 24 * 7 ) {
            QFile reviewFile(fileName);
            if (reviewFile.open(QIODevice::ReadOnly)) {
                QByteArray reviews = reviewFile.readAll();
                QJsonDocument document = QJsonDocument::fromJson(reviews);
                parseReviews(document, app);
                return;
            }
        }
    }

    const QJsonDocument document(QJsonObject{
            {QStringLiteral("app_id"), app->appstreamId()},
            {QStringLiteral("distro"), osName()},
            {QStringLiteral("user_hash"), userHash()},
            {QStringLiteral("version"), app->isInstalled() ? app->installedVersion() : app->availableVersion()},
            {QStringLiteral("locale"), QLocale::system().name()},
            {QStringLiteral("limit"), 0}
    });

    QNetworkRequest request(QUrl(QStringLiteral(APIURL "/fetch")));
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json; charset=utf-8"));
    request.setHeader(QNetworkRequest::ContentLengthHeader, document.toJson().size());
    // Store reference to the app for which we request reviews
    request.setOriginatingObject(app);

    auto reply = m_nam->post(request, document.toJson());
    connect(reply, &QNetworkReply::finished, this, &OdrsReviewsBackend::reviewsFetched);
}

void OdrsReviewsBackend::reviewsFetched()
{
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());

    if (reply->error() == QNetworkReply::NoError) {
        QByteArray data = reply->readAll();
        const QJsonDocument document = QJsonDocument::fromJson(data);
        AbstractResource *resource = qobject_cast<AbstractResource*>(reply->request().originatingObject());
        Q_ASSERT(resource);
        parseReviews(document, resource);

        // Store reviews to cache so we don't need to download them all the time
        if (document.array().isEmpty()) {
            return;
        }

        QJsonObject jsonObject = document.array().first().toObject();
        if (jsonObject.isEmpty()) {
            return;
        }

        QFile file(QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + QStringLiteral("/reviews/%1.json").arg(jsonObject.value(QStringLiteral("app_id")).toString()));
        if (file.open(QIODevice::WriteOnly)) {
            file.write(document.toJson());
            file.close();
        }
    } else {
        m_isFetching = false;
    }
}

Rating * OdrsReviewsBackend::ratingForApplication(AbstractResource *app) const
{
    if (app->isTechnical()) {
        return nullptr;
    }

    return m_ratings[app->appstreamId()];
}

void OdrsReviewsBackend::submitUsefulness(Review *review, bool useful)
{
    const QJsonDocument document(QJsonObject{
                     {QStringLiteral("app_id"), review->applicationName()},
                     {QStringLiteral("user_skey"), review->getMetadata(QStringLiteral("ODRS::user_skey")).toString()},
                     {QStringLiteral("user_hash"), userHash()},
                     {QStringLiteral("distro"), osName()},
                     {QStringLiteral("review_id"), QJsonValue(double(review->id()))} //if we really need uint64 we should get it in QJsonValue
    });

    QNetworkRequest request(QUrl(QStringLiteral(APIURL "/%1").arg(useful ? QStringLiteral("upvote") : QStringLiteral("downvote"))));
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json; charset=utf-8"));
    request.setHeader(QNetworkRequest::ContentLengthHeader, document.toJson().size());

    auto reply = m_nam->post(request, document.toJson());
    connect(reply, &QNetworkReply::finished, this, &OdrsReviewsBackend::usefulnessSubmitted);
}

void OdrsReviewsBackend::usefulnessSubmitted()
{
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());

    if (reply->error() == QNetworkReply::NoError) {
        qWarning() << "Usefullness submitted";
    } else {
        qWarning() << "Failed to submit usefulness: " << reply->errorString();
    }
}

void OdrsReviewsBackend::submitReview(AbstractResource *res, const QString &summary, const QString &description, const QString &rating)
{
    QJsonObject map = {{QStringLiteral("app_id"), res->appstreamId()},
                     {QStringLiteral("user_skey"), res->getMetadata(QStringLiteral("ODRS::user_skey")).toString()},
                     {QStringLiteral("user_hash"), userHash()},
                     {QStringLiteral("version"), res->isInstalled() ? res->installedVersion() : res->availableVersion()},
                     {QStringLiteral("locale"), QLocale::system().name()},
                     {QStringLiteral("distro"), osName()},
                     {QStringLiteral("user_display"), QJsonValue::fromVariant(KUser().property(KUser::FullName))},
                     {QStringLiteral("summary"), summary},
                     {QStringLiteral("description"), description},
                     {QStringLiteral("rating"), rating.toInt() * 10}};

    const QJsonDocument document(map);

    QNetworkAccessManager *accessManager = new QNetworkAccessManager(this);
    QNetworkRequest request(QUrl(QStringLiteral(APIURL "/submit")));
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json; charset=utf-8"));
    request.setHeader(QNetworkRequest::ContentLengthHeader, document.toJson().size());

    // Store what we need so we can immediately show our review once it is submitted
    // Use review_id 0 for now as odrs starts numbering from 1 and once reviews are re-downloaded we get correct id
    map.insert(QStringLiteral("review_id"), 0);
    res->addMetadata(QStringLiteral("ODRS::review_map"), map);
    request.setOriginatingObject(res);

    accessManager->post(request, document.toJson());
    connect(accessManager, &QNetworkAccessManager::finished, this, &OdrsReviewsBackend::reviewSubmitted);
}

void OdrsReviewsBackend::reviewSubmitted(QNetworkReply *reply)
{
    if (reply->error() == QNetworkReply::NoError) {
        qWarning() << "Review submitted";
        AbstractResource *resource = qobject_cast<AbstractResource*>(reply->request().originatingObject());
        const QJsonArray array = {resource->getMetadata(QStringLiteral("ODRS::review_map")).toObject()};
        QJsonDocument document = QJsonDocument(array);
        // Remove local file with reviews so we can re-download it next time to get our review
        QFile file(QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + QStringLiteral("/reviews/%1.json").arg(array.first().toObject().value(QStringLiteral("app_id")).toString()));
        file.remove();
        parseReviews(document, resource);
    } else {
        qWarning() << "Failed to submit review: " << reply->errorString();
    }
}

void OdrsReviewsBackend::parseRatings()
{
    QFile ratingsDocument(QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + QStringLiteral("/ratings/ratings"));
    if (ratingsDocument.open(QIODevice::ReadOnly)) {
        QJsonDocument jsonDocument = QJsonDocument::fromJson(ratingsDocument.readAll());
        QJsonObject jsonObject = jsonDocument.object();
        for (auto it = jsonObject.begin(); it != jsonObject.end(); it++) {
            QJsonObject appJsonObject = it.value().toObject();

            const int ratingCount =  appJsonObject.value(QLatin1String("total")).toInt();
            QVariantMap ratingMap = { { QStringLiteral("star0"), appJsonObject.value(QLatin1String("star0")).toInt() },
                                      { QStringLiteral("star1"), appJsonObject.value(QLatin1String("star1")).toInt() },
                                      { QStringLiteral("star2"), appJsonObject.value(QLatin1String("star2")).toInt() },
                                      { QStringLiteral("star3"), appJsonObject.value(QLatin1String("star3")).toInt() },
                                      { QStringLiteral("star4"), appJsonObject.value(QLatin1String("star4")).toInt() },
                                      { QStringLiteral("star5"), appJsonObject.value(QLatin1String("star5")).toInt() } };

            Rating *rating = new Rating(it.key(), ratingCount, ratingMap);
            m_ratings.insert(it.key(), rating);
        }
        ratingsDocument.close();

        Q_EMIT ratingsReady();
    }
}

void OdrsReviewsBackend::parseReviews(const QJsonDocument &document, AbstractResource *resource)
{
    m_isFetching = false;

    QJsonArray reviews = document.array();
    if (!reviews.isEmpty()) {
        QVector<ReviewPtr> reviewList;
        for (auto it = reviews.begin(); it != reviews.end(); it++) {
            QJsonObject review = it->toObject();
            if (!review.isEmpty()) {
                const int usefulFavorable = review.value(QStringLiteral("karma_up")).toInt();
                const int usefulTotal = review.value(QStringLiteral("karma_down")).toInt() + usefulFavorable;
                QDateTime dateTime;
                dateTime.setTime_t(review.value(QStringLiteral("date_created")).toInt());
                ReviewPtr r(new Review(review.value(QStringLiteral("app_id")).toString(), resource->packageName(),
                                       review.value(QStringLiteral("locale")).toString(), review.value(QStringLiteral("summary")).toString(),
                                       review.value(QStringLiteral("description")).toString(), review.value(QStringLiteral("user_display")).toString(),
                                       dateTime, true, review.value(QStringLiteral("review_id")).toInt(),
                                       review.value(QStringLiteral("rating")).toInt() / 10, usefulTotal, usefulFavorable,
                                       review.value(QStringLiteral("version")).toString()));
                // We can also receive just a json with app name and user info so filter these out as there is no review
                if (!r->summary().isEmpty() && !r->reviewText().isEmpty()) {
                    reviewList << r;
                    // Needed for submitting usefulness
                    r->addMetadata(QStringLiteral("ODRS::user_skey"), review.value(QStringLiteral("user_skey")).toString());
                }

                // We should get at least user_skey needed for posting reviews
                resource->addMetadata(QStringLiteral("ODRS::user_skey"), review.value(QStringLiteral("user_skey")).toString());
            }
        }

        Q_EMIT reviewsReady(resource, reviewList, false);
    }
}

bool OdrsReviewsBackend::isResourceSupported(AbstractResource* res) const
{
    return !res->appstreamId().isEmpty();
}
