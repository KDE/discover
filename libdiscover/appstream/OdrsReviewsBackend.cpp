/*
 *   SPDX-FileCopyrightText: 2013 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *   SPDX-FileCopyrightText: 2017 Jan Grulich <jgrulich@redhat.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "OdrsReviewsBackend.h"
#include "AppStreamIntegration.h"
#include "CachedNetworkAccessManager.h"

#include <ReviewsBackend/Rating.h>
#include <ReviewsBackend/Review.h>

#include <resources/AbstractResource.h>
#include <resources/AbstractResourcesBackend.h>

#include <KIO/FileCopyJob>
#include <KLocalizedString>
#include <KUser>

#include "libdiscover_debug.h"
#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QStandardPaths>

#include <QFutureWatcher>
#include <QtConcurrentRun>

// #define APIURL "http://127.0.0.1:5000/1.0/reviews/api"
#define APIURL "https://odrs.gnome.org/1.0/reviews/api"

OdrsReviewsBackend::OdrsReviewsBackend()
    : AbstractReviewsBackend(nullptr)
{
    bool fetchRatings = false;
    const QUrl ratingsUrl(QStringLiteral(APIURL "/ratings"));
    const QUrl fileUrl = QUrl::fromLocalFile(QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + QStringLiteral("/ratings/ratings"));
    const QDir cacheDir(QStandardPaths::writableLocation(QStandardPaths::CacheLocation));

    // Create $HOME/.cache/discover/ratings folder
    cacheDir.mkpath(QStringLiteral("ratings"));

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
        setFetching(true);
        KIO::FileCopyJob *getJob = KIO::file_copy(ratingsUrl, fileUrl, -1, KIO::Overwrite | KIO::HideProgressInfo);
        connect(getJob, &KIO::FileCopyJob::result, this, &OdrsReviewsBackend::ratingsFetched);
    } else {
        parseRatings();
    }
}

OdrsReviewsBackend::~OdrsReviewsBackend() noexcept
{
    qDeleteAll(m_ratings);
}

void OdrsReviewsBackend::setFetching(bool fetching)
{
    if (fetching == m_isFetching) {
        return;
    }
    m_isFetching = fetching;
    Q_EMIT fetchingChanged(fetching);
}

void OdrsReviewsBackend::ratingsFetched(KJob *job)
{
    setFetching(false);
    if (job->error()) {
        qCWarning(LIBDISCOVER_LOG) << "Failed to fetch ratings " << job->errorString();
    } else {
        parseRatings();
    }
}

static QString osName()
{
    return AppStreamIntegration::global()->osRelease()->name();
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

    QString salted = QStringLiteral("gnome-software[%1:%2]").arg(KUser().loginName(), machineId);
    return QString::fromUtf8(QCryptographicHash::hash(salted.toUtf8(), QCryptographicHash::Sha1).toHex());
}

void OdrsReviewsBackend::fetchReviews(AbstractResource *app, int page)
{
    if (app->appstreamId().isEmpty()) {
        return;
    }
    Q_UNUSED(page)
    const QString version = app->isInstalled() ? app->installedVersion() : app->availableVersion();
    if (version.isEmpty()) {
        return;
    }
    setFetching(true);

    const QJsonDocument document(QJsonObject{
        {QStringLiteral("app_id"), app->appstreamId()},
        {QStringLiteral("distro"), osName()},
        {QStringLiteral("user_hash"), userHash()},
        {QStringLiteral("version"), version},
        {QStringLiteral("locale"), QLocale::system().name()},
        {QStringLiteral("limit"), -1},
    });

    const auto json = document.toJson(QJsonDocument::Compact);
    QNetworkRequest request(QUrl(QStringLiteral(APIURL "/fetch")));
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json; charset=utf-8"));
    request.setHeader(QNetworkRequest::ContentLengthHeader, json.size());
    // Store reference to the app for which we request reviews
    request.setOriginatingObject(app);

    auto reply = nam()->post(request, json);
    connect(reply, &QNetworkReply::finished, this, &OdrsReviewsBackend::reviewsFetched);
}

void OdrsReviewsBackend::reviewsFetched()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    QScopedPointer<QNetworkReply, QScopedPointerDeleteLater> replyPtr(reply);
    const QByteArray data = reply->readAll();
    const auto networkError = reply->error();
    if (networkError != QNetworkReply::NoError) {
        qCWarning(LIBDISCOVER_LOG) << "error fetching reviews:" << reply->errorString() << data;
        Q_EMIT error(i18n("Error while fetching reviews: %1", reply->errorString()));
        setFetching(false);
        return;
    }

    const QJsonDocument document = QJsonDocument::fromJson(data);
    AbstractResource *resource = qobject_cast<AbstractResource *>(reply->request().originatingObject());
    Q_ASSERT(resource);
    parseReviews(document, resource);
}

Rating *OdrsReviewsBackend::ratingForApplication(AbstractResource *app) const
{
    if (app->appstreamId().isEmpty()) {
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
        {QStringLiteral("review_id"), QJsonValue(double(review->id()))}, // if we really need uint64 we should get it in QJsonValue
    });

    QNetworkRequest request(QUrl(QStringLiteral(APIURL) + (useful ? QLatin1String("/upvote") : QLatin1String("/downvote"))));
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json; charset=utf-8"));
    request.setHeader(QNetworkRequest::ContentLengthHeader, document.toJson().size());

    auto reply = nam()->post(request, document.toJson());
    connect(reply, &QNetworkReply::finished, this, &OdrsReviewsBackend::usefulnessSubmitted);
}

void OdrsReviewsBackend::usefulnessSubmitted()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    const auto networkError = reply->error();
    if (networkError == QNetworkReply::NoError) {
        qCWarning(LIBDISCOVER_LOG) << "Usefulness submitted";
    } else {
        qCWarning(LIBDISCOVER_LOG) << "Failed to submit usefulness: " << reply->errorString();
        Q_EMIT error(i18n("Error while submitting usefulness: %1", reply->errorString()));
    }
    reply->deleteLater();
}

QString OdrsReviewsBackend::userName() const
{
    return KUser().property(KUser::FullName).toString();
}

void OdrsReviewsBackend::sendReview(AbstractResource *res, const QString &summary, const QString &description, const QString &rating, const QString &userName)
{
    Q_ASSERT(res);
    QJsonObject map = {
        {QStringLiteral("app_id"), res->appstreamId()},
        {QStringLiteral("user_skey"), res->getMetadata(QStringLiteral("ODRS::user_skey")).toString()},
        {QStringLiteral("user_hash"), userHash()},
        {QStringLiteral("version"), res->isInstalled() ? res->installedVersion() : res->availableVersion()},
        {QStringLiteral("locale"), QLocale::system().name()},
        {QStringLiteral("distro"), osName()},
        {QStringLiteral("user_display"), QJsonValue::fromVariant(userName)},
        {QStringLiteral("summary"), summary},
        {QStringLiteral("description"), description},
        {QStringLiteral("rating"), rating.toInt() * 10},
    };

    const QJsonDocument document(map);

    QNetworkAccessManager *accessManager = nam();
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
    const auto networkError = reply->error();
    if (networkError == QNetworkReply::NoError) {
        AbstractResource *resource = qobject_cast<AbstractResource *>(reply->request().originatingObject());
        Q_ASSERT(resource);
        qCWarning(LIBDISCOVER_LOG) << "Review submitted" << resource;
        if (resource) {
            const QJsonDocument document({resource->getMetadata(QStringLiteral("ODRS::review_map")).toObject()});
            parseReviews(document, resource);
        } else {
            qCWarning(LIBDISCOVER_LOG) << "Failed to submit review: missing object";
        }
    } else {
        Q_EMIT error(i18n("Error while submitting review: %1", reply->errorString()));
        qCWarning(LIBDISCOVER_LOG) << "Failed to submit review: " << reply->errorString();
    }
    reply->deleteLater();
}

void OdrsReviewsBackend::parseRatings()
{
    auto fw = new QFutureWatcher<QJsonDocument>(this);
    connect(fw, &QFutureWatcher<QJsonDocument>::finished, this, [this, fw] {
        const QJsonDocument jsonDocument = fw->result();
        fw->deleteLater();
        const QJsonObject jsonObject = jsonDocument.object();
        m_ratings.reserve(jsonObject.size());
        for (auto it = jsonObject.begin(); it != jsonObject.end(); it++) {
            QJsonObject appJsonObject = it.value().toObject();

            const int ratingCount = appJsonObject.value(QLatin1String("total")).toInt();
            int ratingMap[] = {
                appJsonObject.value(QLatin1String("star0")).toInt(),
                appJsonObject.value(QLatin1String("star1")).toInt(),
                appJsonObject.value(QLatin1String("star2")).toInt(),
                appJsonObject.value(QLatin1String("star3")).toInt(),
                appJsonObject.value(QLatin1String("star4")).toInt(),
                appJsonObject.value(QLatin1String("star5")).toInt(),
            };

            Rating *rating = new Rating(it.key(), ratingCount, ratingMap);
            m_ratings.insert(it.key(), rating);
        }
        Q_EMIT ratingsReady();
    });
    fw->setFuture(QtConcurrent::run([] {
        QFile ratingsDocument(QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + QStringLiteral("/ratings/ratings"));
        if (!ratingsDocument.open(QIODevice::ReadOnly)) {
            return QJsonDocument::fromJson({});
        }
        return QJsonDocument::fromJson(ratingsDocument.readAll());
    }));
}

void OdrsReviewsBackend::parseReviews(const QJsonDocument &document, AbstractResource *resource)
{
    setFetching(false);
    Q_ASSERT(resource);
    if (!resource) {
        return;
    }

    QJsonArray reviews = document.array();
    if (!reviews.isEmpty()) {
        QVector<ReviewPtr> reviewList;
        for (auto it = reviews.begin(); it != reviews.end(); it++) {
            const QJsonObject review = it->toObject();
            if (!review.isEmpty()) {
                const int usefulFavorable = review.value(QStringLiteral("karma_up")).toInt();
                const int usefulTotal = review.value(QStringLiteral("karma_down")).toInt() + usefulFavorable;
                QDateTime dateTime;
                dateTime.setSecsSinceEpoch(review.value(QStringLiteral("date_created")).toInt());
                ReviewPtr r(new Review(review.value(QStringLiteral("app_id")).toString(),
                                       resource->packageName(),
                                       review.value(QStringLiteral("locale")).toString(),
                                       review.value(QStringLiteral("summary")).toString(),
                                       review.value(QStringLiteral("description")).toString(),
                                       review.value(QStringLiteral("user_display")).toString(),
                                       dateTime,
                                       true,
                                       review.value(QStringLiteral("review_id")).toInt(),
                                       review.value(QStringLiteral("rating")).toInt() / 10,
                                       usefulTotal,
                                       usefulFavorable,
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

bool OdrsReviewsBackend::isResourceSupported(AbstractResource *res) const
{
    return !res->appstreamId().isEmpty();
}

void OdrsReviewsBackend::emitRatingFetched(AbstractResourcesBackend *b, const QList<AbstractResource *> &resources) const
{
    b->emitRatingsReady();
    for (AbstractResource *res : resources) {
        if (m_ratings.contains(res->appstreamId())) {
            Q_EMIT res->ratingFetched();
        }
    }
}

QNetworkAccessManager *OdrsReviewsBackend::nam()
{
    if (!m_delayedNam) {
        m_delayedNam = new CachedNetworkAccessManager(QStringLiteral("odrs"), this);
    }
    return m_delayedNam;
}
