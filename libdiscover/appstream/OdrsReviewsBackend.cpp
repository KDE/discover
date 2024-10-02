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

#include <qnumeric.h>
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

QSharedPointer<OdrsReviewsBackend> OdrsReviewsBackend::global()
{
    static QSharedPointer<OdrsReviewsBackend> var = nullptr;
    if (!var) {
        var = QSharedPointer<OdrsReviewsBackend>(new OdrsReviewsBackend());
    }

    return var;
}

OdrsReviewsBackend::OdrsReviewsBackend()
    : AbstractReviewsBackend(nullptr)
{
    fetchRatings();
}

OdrsReviewsBackend::~OdrsReviewsBackend()
{
}

void OdrsReviewsBackend::fetchRatings()
{
    bool fetchRatings = false;
    const QUrl ratingsUrl(QLatin1StringView(APIURL "/ratings"));
    const QUrl fileUrl = QUrl::fromLocalFile(QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + QLatin1StringView("/ratings/ratings"));
    const QDir cacheDir(QStandardPaths::writableLocation(QStandardPaths::CacheLocation));

    // Create $HOME/.cache/discover/ratings folder
    cacheDir.mkpath(QLatin1StringView("ratings"));

    if (QFileInfo::exists(fileUrl.toLocalFile())) {
        QFileInfo file(fileUrl.toLocalFile());
        // Refresh the cached ratings if they are older than one day
        if (file.lastModified().msecsTo(QDateTime::currentDateTime()) > 1000 * 60 * 60 * 24) {
            fetchRatings = true;
        }
    } else {
        fetchRatings = true;
    }

    qCWarning(LIBDISCOVER_LOG) << "OdrsReviewsBackend: Fetch ratings:" << fetchRatings;
    if (fetchRatings) {
        setFetching(true);
        auto getJob = KIO::file_copy(ratingsUrl, fileUrl, -1, KIO::Overwrite | KIO::HideProgressInfo);
        connect(getJob, &KIO::FileCopyJob::result, this, &OdrsReviewsBackend::ratingsFetched);
    } else {
        parseRatings();
    }
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
        qCWarning(LIBDISCOVER_LOG) << "OdrsReviewsBackend: Failed to fetch ratings:" << job->errorString();
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
    QByteArray machineId;
    QFile file(QLatin1StringView("/etc/machine-id"));
    if (file.open(QIODevice::ReadOnly)) {
        machineId = file.readAll();
        file.close();
    }

    if (machineId.isEmpty()) {
        return QString();
    }

    const QByteArray salted = QByteArray("gnome-software[") + KUser().loginName().toUtf8() + ':' + machineId + ']';
    return QString::fromUtf8(QCryptographicHash::hash(salted, QCryptographicHash::Sha1).toHex());
}

void OdrsReviewsBackend::fetchReviews(AbstractResource *resource, int page)
{
    if (resource->appstreamId().isEmpty()) {
        return;
    }
    Q_UNUSED(page)
    QString version = resource->isInstalled() ? resource->installedVersion() : resource->availableVersion();
    if (version.isEmpty()) {
        version = QLatin1StringView("unknown");
    }
    setFetching(true);

    const QJsonDocument document(QJsonObject{
        {QLatin1StringView("app_id"), resource->appstreamId()},
        {QLatin1StringView("distro"), osName()},
        {QLatin1StringView("user_hash"), userHash()},
        {QLatin1StringView("version"), version},
        {QLatin1StringView("locale"), QLocale::system().name()},
        {QLatin1StringView("limit"), -1},
    });

    const auto json = document.toJson(QJsonDocument::Compact);
    QNetworkRequest request(QUrl(QStringLiteral(APIURL "/fetch")));
    request.setHeader(QNetworkRequest::ContentTypeHeader, QLatin1StringView("application/json; charset=utf-8"));
    request.setHeader(QNetworkRequest::ContentLengthHeader, json.size());
    // Store reference to the resource for which we request reviews
    request.setOriginatingObject(resource);

    auto reply = nam()->post(request, json);
    connect(reply, &QNetworkReply::finished, this, &OdrsReviewsBackend::reviewsFetched);
}

void OdrsReviewsBackend::reviewsFetched()
{
    const auto reply = qobject_cast<QNetworkReply *>(sender());
    QScopedPointer<QNetworkReply, QScopedPointerDeleteLater> replyPtr(reply);
    const QByteArray data = reply->readAll();
    const auto networkError = reply->error();
    if (networkError != QNetworkReply::NoError) {
        qCWarning(LIBDISCOVER_LOG) << "OdrsReviewsBackend: Error fetching reviews:" << reply->errorString() << data;
        m_errorMessage = i18n("Technical error message: %1", reply->errorString());
        Q_EMIT errorMessageChanged();
        setFetching(false);
        return;
    }

    QJsonParseError error;
    const QJsonDocument document = QJsonDocument::fromJson(data, &error);
    if (error.error) {
        qCWarning(LIBDISCOVER_LOG) << "OdrsReviewsBackend: Error parsing reviews:" << reply->url() << error.errorString();
    }

    const auto resource = qobject_cast<AbstractResource *>(reply->request().originatingObject());
    Q_ASSERT(resource);
    parseReviews(document, resource);
}

Rating OdrsReviewsBackend::ratingForApplication(AbstractResource *resource) const
{
    if (resource->appstreamId().isEmpty()) {
        return {};
    }

    return m_current.ratings[resource->appstreamId()];
}

void OdrsReviewsBackend::submitUsefulness(Review *review, bool useful)
{
    const QJsonDocument document(QJsonObject{
        {QLatin1StringView("app_id"), review->applicationName()},
        {QLatin1StringView("user_skey"), review->getMetadata(QLatin1StringView("ODRS::user_skey")).toString()},
        {QLatin1StringView("user_hash"), userHash()},
        {QLatin1StringView("distro"), osName()},
        {QLatin1StringView("review_id"), QJsonValue(double(review->id()))}, // if we really need uint64 we should get it in QJsonValue
    });

    QNetworkRequest request(QUrl(QStringLiteral(APIURL) + (useful ? QLatin1String("/upvote") : QLatin1String("/downvote"))));
    request.setHeader(QNetworkRequest::ContentTypeHeader, QLatin1StringView("application/json; charset=utf-8"));
    request.setHeader(QNetworkRequest::ContentLengthHeader, document.toJson().size());

    auto reply = nam()->post(request, document.toJson());
    connect(reply, &QNetworkReply::finished, this, &OdrsReviewsBackend::usefulnessSubmitted);
}

void OdrsReviewsBackend::usefulnessSubmitted()
{
    const auto reply = qobject_cast<QNetworkReply *>(sender());
    const auto networkError = reply->error();
    if (networkError == QNetworkReply::NoError) {
        qCWarning(LIBDISCOVER_LOG) << "OdrsReviewsBackend: Usefulness submitted successfully";
    } else {
        qCWarning(LIBDISCOVER_LOG).noquote() << "OdrsReviewsBackend: Failed to submit usefulness:" << reply->errorString();
        Q_EMIT error(i18n("Error while submitting usefulness: %1", reply->errorString()));
    }
    reply->deleteLater();
}

QString OdrsReviewsBackend::userName() const
{
    return KUser().property(KUser::FullName).toString();
}

void OdrsReviewsBackend::sendReview(AbstractResource *resource,
                                    const QString &summary,
                                    const QString &reviewText,
                                    const QString &rating,
                                    const QString &userName)
{
    Q_ASSERT(resource);
    QJsonObject map = {
        {QLatin1StringView("app_id"), resource->appstreamId()},
        {QLatin1StringView("user_skey"), resource->getMetadata(QLatin1StringView("ODRS::user_skey")).toString()},
        {QLatin1StringView("user_hash"), userHash()},
        {QLatin1StringView("version"), resource->isInstalled() ? resource->installedVersion() : resource->availableVersion()},
        {QLatin1StringView("locale"), QLocale::system().name()},
        {QLatin1StringView("distro"), osName()},
        {QLatin1StringView("user_display"), QJsonValue::fromVariant(userName)},
        {QLatin1StringView("summary"), summary},
        {QLatin1StringView("description"), reviewText},
        {QLatin1StringView("rating"), rating.toInt() * 10},
    };

    const QJsonDocument document(map);

    const auto accessManager = nam();
    QNetworkRequest request(QUrl(QStringLiteral(APIURL "/submit")));
    request.setHeader(QNetworkRequest::ContentTypeHeader, QLatin1StringView("application/json; charset=utf-8"));
    request.setHeader(QNetworkRequest::ContentLengthHeader, document.toJson().size());

    // Store what we need so we can immediately show our review once it is submitted
    // Use review_id 0 for now as odrs starts numbering from 1 and once reviews are re-downloaded we get correct id
    map.insert(QLatin1StringView("review_id"), 0);
    resource->addMetadata(QLatin1StringView("ODRS::review_map"), map);
    request.setOriginatingObject(resource);

    accessManager->post(request, document.toJson());
    connect(accessManager, &QNetworkAccessManager::finished, this, &OdrsReviewsBackend::reviewSubmitted);
}

void OdrsReviewsBackend::reviewSubmitted(QNetworkReply *reply)
{
    const auto networkError = reply->error();
    if (networkError == QNetworkReply::NoError) {
        const auto resource = qobject_cast<AbstractResource *>(reply->request().originatingObject());
        Q_ASSERT(resource);
        qCWarning(LIBDISCOVER_LOG) << "OdrsReviewsBackend: Review submitted for" << resource;
        if (resource) {
            const QJsonDocument document({resource->getMetadata(QLatin1StringView("ODRS::review_map")).toObject()});
            parseReviews(document, resource);
        } else {
            qCWarning(LIBDISCOVER_LOG) << "OdrsReviewsBackend: Failed to submit review: missing object";
        }
    } else {
        qCWarning(LIBDISCOVER_LOG).noquote() << "OdrsReviewsBackend: Failed to submit review:" << reply->error() << reply->errorString()
                                             << reply->rawHeaderPairs();
        Q_EMIT error(i18n("Error while submitting review: %1", reply->errorString()));
    }
    reply->deleteLater();
}

void OdrsReviewsBackend::parseRatings()
{
    auto fw = new QFutureWatcher<State>(this);
    connect(fw, &QFutureWatcher<State>::finished, this, [this, fw] {
        fw->deleteLater();
        m_current = fw->result();
        Q_EMIT ratingsReady();
    });
    fw->setFuture(QtConcurrent::run([]() -> State {
        QFile ratingsDocument(QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + QLatin1StringView("/ratings/ratings"));
        if (!ratingsDocument.open(QIODevice::ReadOnly)) {
            qCWarning(LIBDISCOVER_LOG) << "OdrsReviewsBackend: Could not open file" << ratingsDocument.fileName();
            return {};
        }

        QJsonParseError error;
        const auto jsonDocument = QJsonDocument::fromJson(ratingsDocument.readAll(), &error);
        if (error.error) {
            qCWarning(LIBDISCOVER_LOG) << "OdrsReviewsBackend: Error parsing ratings:" << ratingsDocument.errorString() << error.errorString();
        }

        State state;
        const auto jsonObject = jsonDocument.object();
        state.ratings.reserve(jsonObject.size());
        constexpr uint topSize = 25;
        for (auto it = jsonObject.begin(); it != jsonObject.end(); it++) {
            const auto appJsonObject = it.value().toObject();

            const int ratingCount = appJsonObject.value(QLatin1String("total")).toInt();
            int ratingMap[] = {
                appJsonObject.value(QLatin1String("star0")).toInt(),
                appJsonObject.value(QLatin1String("star1")).toInt(),
                appJsonObject.value(QLatin1String("star2")).toInt(),
                appJsonObject.value(QLatin1String("star3")).toInt(),
                appJsonObject.value(QLatin1String("star4")).toInt(),
                appJsonObject.value(QLatin1String("star5")).toInt(),
            };

            const auto rating = Rating(it.key(), ratingCount, ratingMap);
            const auto finder = [&rating](const Rating &review) {
                return review.ratingPoints() < rating.ratingPoints();
            };
            const auto topIt = std::find_if(state.top.begin(), state.top.end(), finder);
            if (topIt == state.top.end()) {
                if (state.top.size() < topSize) {
                    state.top.append(rating);
                }
            } else {
                state.top.insert(topIt, rating);
            }
            if (state.top.size() > topSize) {
                state.top.resize(topSize);
            }

            state.ratings.insert(it.key(), rating);
        }
        return state;
    }));
}

void OdrsReviewsBackend::parseReviews(const QJsonDocument &document, AbstractResource *resource)
{
    setFetching(false);
    Q_ASSERT(resource);
    if (!resource) {
        return;
    }

    const auto reviews = document.array();
    if (!reviews.isEmpty()) {
        QList<ReviewPtr> reviewsList;
        for (const auto &it : reviews) {
            const QJsonObject review = it.toObject();
            if (!review.isEmpty()) {
                // Same ranking algorythm Gnome Software uses
                const int usefulFavorable = review.value(QLatin1StringView("karma_up")).toInt();
                const int usefulNegative = review.value(QLatin1StringView("karma_down")).toInt();
                const int usefulTotal = usefulFavorable + usefulNegative;

                qreal usefulWilson = 0.0;

                /* from http://www.evanmiller.org/how-not-to-sort-by-average-rating.html */
                if (usefulFavorable > 0 || usefulNegative > 0) {
                    usefulWilson = ((usefulFavorable + 1.9208) / (usefulFavorable + usefulNegative)
                                    - 1.96 * sqrt((usefulFavorable * usefulNegative) / qreal(usefulFavorable + usefulNegative) + 0.9604)
                                        / (usefulFavorable + usefulNegative))
                        / (1 + 3.8416 / (usefulFavorable + usefulNegative));
                    usefulWilson *= 100.0;
                }

                QDateTime dateTime;
                dateTime.setSecsSinceEpoch(review.value(QLatin1StringView("date_created")).toInt());

                // If there is no score or the score is the same, base on the age
                const auto currentDateTime = QDateTime::currentDateTime();
                const auto totalDays = static_cast<qreal>(dateTime.daysTo(currentDateTime));

                // use also the longest common subsequence of the version string to compute relevance
                const auto reviewVersion = review.value(QLatin1StringView("version")).toString();
                const auto availableVersion = resource->availableVersion();
                qreal versionScore = 0;
                const int minLength = std::min(reviewVersion.length(), availableVersion.length());
                if (minLength > 0) {
                    for (int i = 0; i < minLength; ++i) {
                        if (reviewVersion[i] != availableVersion[i] || i == minLength - 1) {
                            versionScore = i;
                            break;
                        }
                    }
                    // Normalize
                    versionScore = versionScore / qreal(std::max(reviewVersion.length(), availableVersion.length()) - 1);
                }

                // Very random heuristic which weights usefulness with age and version similarity. Don't penalize usefulness more than 6 months
                usefulWilson = versionScore + 1.0 / std::max(1.0, totalDays) + usefulWilson / std::clamp(totalDays, 1.0, 93.0);

                const bool shouldShow = usefulFavorable >= usefulNegative * 2 && review.value(QLatin1StringView("reported")).toInt() < 4;
                ReviewPtr r(new Review(review.value(QLatin1StringView("app_id")).toString(),
                                       resource->packageName(),
                                       review.value(QLatin1StringView("locale")).toString(),
                                       review.value(QLatin1StringView("summary")).toString(),
                                       review.value(QLatin1StringView("description")).toString(),
                                       review.value(QLatin1StringView("user_display")).toString(),
                                       dateTime,
                                       shouldShow,
                                       review.value(QLatin1StringView("review_id")).toInt(),
                                       review.value(QLatin1StringView("rating")).toInt() / 10,
                                       usefulTotal,
                                       usefulFavorable,
                                       usefulWilson,
                                       reviewVersion));
                // We can also receive just a json with app name and user info so filter these out as there is no review
                if (!r->summary().isEmpty() && !r->reviewText().isEmpty()) {
                    reviewsList.append(r);
                    // Needed for submitting usefulness
                    r->addMetadata(QLatin1StringView("ODRS::user_skey"), review.value(QLatin1StringView("user_skey")).toString());
                }

                // We should get at least user_skey needed for posting reviews
                resource->addMetadata(QLatin1StringView("ODRS::user_skey"), review.value(QLatin1StringView("user_skey")).toString());
            }
        }

        Q_EMIT reviewsReady(resource, reviewsList, false);
    }
}

bool OdrsReviewsBackend::isResourceSupported(AbstractResource *resource) const
{
    return !resource->appstreamId().isEmpty();
}

void OdrsReviewsBackend::emitRatingFetched(AbstractResourcesBackend *backend, const QList<AbstractResource *> &resources) const
{
    backend->emitRatingsReady();
    for (const auto resource : resources) {
        if (m_current.ratings.contains(resource->appstreamId())) {
            Q_EMIT resource->ratingFetched();
        }
    }
}

QNetworkAccessManager *OdrsReviewsBackend::nam()
{
    if (!m_delayedNam) {
        m_delayedNam = new CachedNetworkAccessManager(QLatin1StringView("odrs"), this);
    }
    return m_delayedNam;
}

#include "moc_OdrsReviewsBackend.cpp"
