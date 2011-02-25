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

#include <KIO/Job>
#include <KTemporaryFile>
#include <KUrl>
#include <KDebug>

#include <qjson/parser.h>

#include "../Application.h"
#include "Rating.h"

ReviewsBackend::ReviewsBackend(QObject *parent)
        : QObject(parent)
        , m_serverBase("http://reviews.staging.ubuntu.com/reviews/api/1.0/")
        , m_ratingsFile(0)
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

