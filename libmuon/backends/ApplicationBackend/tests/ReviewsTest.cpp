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
#include <qapt/backend.h>
#include <KProtocolManager>
#include <qtest.h>
#include <ReviewsBackend/AbstractReviewsBackend.h>
#include <MuonBackendsFactory.h>
#include <resources/AbstractResourcesBackend.h>
#include <resources/ResourcesModel.h>
#include <KXmlGuiWindow>
#include <QSignalSpy>

QTEST_MAIN( ReviewsTest )

AbstractResourcesBackend* backendByName(ResourcesModel* m, const QString& name)
{
    QVector<AbstractResourcesBackend*> backends = m->backends();
    foreach(AbstractResourcesBackend* backend, backends) {
        if(backend->metaObject()->className()==name) {
            return backend;
        }
    }
    return nullptr;
}

ReviewsTest::ReviewsTest(QObject* parent): QObject(parent)
{
    ResourcesModel* m = new ResourcesModel("muon-applications-backend", this);
    m_window = new KXmlGuiWindow;
    m->integrateMainWindow(m_window);
    m_appBackend = backendByName(m, "ApplicationBackend");
    QVERIFY(QSignalSpy(m, SIGNAL(allInitialized())).wait());
    m_revBackend = m_appBackend->reviewsBackend();
}

void ReviewsTest::testReviewsFetch()
{
    if(m_revBackend->isFetching())
        QSignalSpy(m_revBackend, SIGNAL(ratingsReady())).wait();
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
    QSignalSpy(model, SIGNAL(rowsInserted(QModelIndex,int,int))).wait(2000);
    
    QModelIndex root;
    while(model->canFetchMore(root)) {
        model->fetchMore(root);
        bool arrived = QSignalSpy(model, SIGNAL(rowsInserted(QModelIndex,int,int))).wait(2000);
        QCOMPARE(arrived, model->canFetchMore(root));
    }
    
    delete model;
}
