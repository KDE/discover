/***************************************************************************
 *   Copyright © 2016 Aleix Pol Gonzalez <aleixpol@blue-systems.com>       *
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

#include "AppstreamReviews.h"
#include <resources/AbstractResourcesBackend.h>

#include <KIO/FileCopyJob>
#include <KCompressionDevice>
#include <QDir>
#include <QStandardPaths>
#include <QUrl>
#include <resources/AbstractResource.h>
#include <ReviewsBackend/PopConParser.h>
#include <ReviewsBackend/Rating.h>
#include <QDebug>

#include "AppPackageKitResource.h"

Q_GLOBAL_STATIC(QUrl, ratingsCache)

AppstreamReviews::AppstreamReviews(AbstractResourcesBackend* parent)
    : AbstractReviewsBackend(parent)
    , m_fetching(true)
{
    const QUrl ratingsUrl(QStringLiteral("http://appstream.kubuntu.co.uk/appstream-ubuntu-popcon-results.gz"));
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
    QDir().mkpath(dir);
    *ratingsCache = QUrl::fromLocalFile(dir+QLatin1String("/appstream-popcon.gz"));

    KIO::FileCopyJob *getJob = KIO::file_copy(ratingsUrl, *ratingsCache, -1,
                               KIO::Overwrite | KIO::HideProgressInfo);
    connect(getJob, &KIO::FileCopyJob::result, this, &AppstreamReviews::ratingsFetched);

    if (QFile::exists(ratingsCache->toLocalFile()))
        readRatings();
}

void AppstreamReviews::ratingsFetched(KJob* job)
{
    m_fetching = false;
    if (job->error()) {
        qWarning() << "error fetching popcon" << job->errorString();
    } else {
        readRatings();
    }
}

void AppstreamReviews::readRatings()
{
    QScopedPointer<QIODevice> dev(new KCompressionDevice(ratingsCache->toLocalFile(), KCompressionDevice::GZip));
    if (!dev->open(QIODevice::ReadOnly)) {
        qWarning() << "couldn't open popcon file" << dev->errorString();
    } else {
        foreach(Rating* res, m_ratings) {
            res->deleteLater();
        }
        m_ratings = PopConParser::parsePopcon(this, dev.data());
        Q_EMIT ratingsReady();
    }
}

Rating * AppstreamReviews::ratingForApplication(AbstractResource* app) const
{
    if (app->isTechnical())
        return nullptr;
    auto appk = qobject_cast<AppPackageKitResource*>(app);
    if (!appk)
        return nullptr;
//     qDebug() << "fuuuuuu" << appk->appstreamId() << m_ratings.value(appk->appstreamId());
    return m_ratings.value(appk->appstreamId());
}

bool AppstreamReviews::isFetching() const
{
    return m_fetching;
}
