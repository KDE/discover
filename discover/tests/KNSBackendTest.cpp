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

#include "KNSBackendTest.h"
#include <KNSBackend/KNSBackend.h>
#include <resources/AbstractResource.h>
#include <resources/ResourcesModel.h>
#include <ReviewsBackend/AbstractReviewsBackend.h>
#include <ReviewsBackend/Rating.h>

#include <qtest_kde.h>

QTEST_KDEMAIN_CORE( KNSBackendTest )

KNSBackendTest::KNSBackendTest(QObject* parent)
    : QObject(parent)
    , m_r(0)
{
    ResourcesModel* model = ResourcesModel::global();
//     m_backend = new KNSBackend("comic.knsrc", "face-smile-big", this);
    m_backend = new KNSBackend("plasmoids.knsrc", "plasma", this);
    model->addResourcesBackend(m_backend);
    QTest::kWaitForSignal(m_backend, SIGNAL(backendReady()));
    connect(m_backend->reviewsBackend(), SIGNAL(reviewsReady(AbstractResource*,QList<Review*>)),
            SLOT(reviewsArrived(AbstractResource*,QList<Review*>)));
}

void KNSBackendTest::testRetrieval()
{
    ResourcesModel* model = ResourcesModel::global();
    QVector<AbstractResource*> resources = m_backend->allResources();
    QVERIFY(!resources.isEmpty());
    QCOMPARE(resources.count(), model->rowCount());
    
    foreach(AbstractResource* res, resources) {
        QVERIFY(!res->name().isEmpty());
        QVERIFY(!res->categories().isEmpty());
    }
}

void KNSBackendTest::testReviews()
{
    QVector<AbstractResource*> resources = m_backend->allResources();
    AbstractReviewsBackend* rev = m_backend->reviewsBackend();
    foreach(AbstractResource* res, resources) {
        Rating* r = rev->ratingForApplication(res);
        QVERIFY(r);
        QCOMPARE(r->packageName(), res->packageName());
        QVERIFY(r->rating()>0 && r->rating()<=10);
//         rev->fetchReviews(res);
//         QTest::kWaitForSignal(rev, SIGNAL(reviewsReady(AbstractResource*,QList<Review*>)));
    }
}

void KNSBackendTest::reviewsArrived(AbstractResource* r, const QList< Review* >& revs)
{
    m_r = r;
    m_revs = revs;
}
