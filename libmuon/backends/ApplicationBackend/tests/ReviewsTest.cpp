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

#include "ReviewsTest.h"
#include "modeltest.h"
#include <ReviewsBackend/ReviewsModel.h>
#include <libqapt/backend.h>
#include <KProtocolManager>
#include <qtest_kde.h>
#include <ReviewsBackend/AbstractReviewsBackend.h>
#include <MuonBackendsFactory.h>
#include <resources/AbstractResourcesBackend.h>
#include <resources/ResourcesModel.h>

QTEST_KDEMAIN_CORE( ReviewsTest )

ReviewsTest::ReviewsTest(QObject* parent): QObject(parent)
{
    ResourcesModel* m = new ResourcesModel("muon-appsbackend", this);
    m_appBackend = m->backends().first();
    QTest::kWaitForSignal(m, SIGNAL(allInitialized()));
    m_revBackend = m_appBackend->reviewsBackend();
}

void ReviewsTest::testReviewsFetch()
{
    if(m_revBackend->isFetching())
        QTest::kWaitForSignal(m_revBackend, SIGNAL(ratingsReady()));
    QVERIFY(!m_revBackend->isFetching());
}

void ReviewsTest::testReviewsModel_data()
{
    QTest::addColumn<QString>( "application" );
    QTest::newRow( "python" ) << "python";
    QTest::newRow( "gedit" ) << "gedit";
}

void ReviewsTest::testReviewsModel()
{
    QFETCH(QString, application);
    ReviewsModel* model = new ReviewsModel(this);
    new ModelTest(model, model);
    
    AbstractResource* app = m_appBackend->resourceByPackageName(application);
    QVERIFY(app);
    model->setResource(app);
    QTest::kWaitForSignal(model, SIGNAL(rowsInserted(QModelIndex,int,int)), 2000);
    
    QModelIndex root;
    while(model->canFetchMore(root)) {
        model->fetchMore(root);
        bool arrived = QTest::kWaitForSignal(model, SIGNAL(rowsInserted(QModelIndex,int,int)), 2000);
        QCOMPARE(arrived, model->canFetchMore(root));
    }
    
    delete model;
}
