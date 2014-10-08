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

#include "DummyTest.h"
#include <modeltest.h>
#include <resources/ResourcesModel.h>
#include <resources/ResourcesProxyModel.h>
#include <resources/AbstractBackendUpdater.h>
#include <ApplicationAddonsModel.h>
#include <Transaction/TransactionModel.h>
#include <ReviewsBackend/ReviewsModel.h>
#include <qtest.h>

#include <QtTest>
#include <QAction>

QTEST_MAIN(DummyTest);

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

DummyTest::DummyTest(QObject* parent): QObject(parent)
{
    m_model = new ResourcesModel("muon-dummy-backend", this);
//     new ModelTest(m_model, m_model);

    m_appBackend = backendByName(m_model, "DummyBackend");
    QVERIFY(m_appBackend);
    QSignalSpy spy(m_appBackend, SIGNAL(backendReady()));
    QVERIFY(spy.wait(0));
}

void DummyTest::testReadData()
{
    QBENCHMARK {
        for(int i=0; i<m_model->rowCount(); i++) {
            QModelIndex idx = m_model->index(i, 0);
            QVERIFY(!m_model->data(idx, ResourcesModel::NameRole).isNull());
        }
        QCOMPARE(m_appBackend->property("startElements").toInt()*2, m_model->rowCount());
    }
}

void DummyTest::testProxy()
{
    ResourcesProxyModel pm;
    pm.setSourceModel(m_model);
    QCOMPARE(m_appBackend->property("startElements").toInt(), pm.rowCount());
    pm.setShouldShowTechnical(true);
    QCOMPARE(m_appBackend->property("startElements").toInt()*2, pm.rowCount());
    pm.setSearch("techie");
    QCOMPARE(m_appBackend->property("startElements").toInt(), pm.rowCount());
    pm.setSearch(QString());
    QCOMPARE(m_appBackend->property("startElements").toInt()*2, pm.rowCount());
}

void DummyTest::testFetch()
{
    QCOMPARE(m_appBackend->property("startElements").toInt()*2, m_model->rowCount());

    //fetches updates, adds new things
    m_appBackend->backendUpdater()->messageActions().first()->trigger();
    QCOMPARE(m_model->rowCount(), 0);
    QCOMPARE(m_model->isFetching(), true);
    QSignalSpy spy(m_model, SIGNAL(allInitialized()));
    QVERIFY(spy.wait(80000));
    QCOMPARE(m_model->isFetching(), false);
    QCOMPARE(m_appBackend->property("startElements").toInt()*4, m_model->rowCount());
}

void DummyTest::testSort()
{
    ResourcesProxyModel pm;
    pm.setSourceModel(m_model);
    QCollator c;
    QBENCHMARK_ONCE {
        pm.setSortRole(ResourcesModel::NameRole);
        pm.sort(0);
        QString last;
        for(int i = 0; i<pm.rowCount(); ++i) {
            QString current = pm.index(i, 0).data(pm.sortRole()).toString();
            if (!last.isEmpty()) {
                QCOMPARE(c.compare(last, current), -1);
            }
            last = current;
        }
    }

    QBENCHMARK_ONCE {
        pm.setSortRole(ResourcesModel::SortableRatingRole);
        pm.sort(0);
        int last=-1;
        for(int i = 0; i<pm.rowCount(); ++i) {
            int current = pm.index(i, 0).data(pm.sortRole()).toInt();
            QVERIFY(last<=current);
            last = current;
        }
    }
}

void DummyTest::testInstallAddons()
{
    AbstractResource* res = m_model->resourceByPackageName("Dummy 1");
    QVERIFY(res);

    ApplicationAddonsModel m;
    m.setApplication(res);
    QCOMPARE(m.rowCount(), res->addonsInformation().count());
    QCOMPARE(res->addonsInformation().first().isInstalled(), false);

    QString firstAddonName = m.data(m.index(0,0)).toString();
    m.changeState(firstAddonName, true);
    QVERIFY(m.hasChanges());

    m.applyChanges();
    QSignalSpy sR(TransactionModel::global(), SIGNAL(transactionRemoved(Transaction* )));
    QVERIFY(sR.wait());
    QVERIFY(!m.hasChanges());

    QCOMPARE(m.data(m.index(0,0)).toString(), firstAddonName);
    QCOMPARE(res->addonsInformation().first().name(), firstAddonName);
    QCOMPARE(res->addonsInformation().first().isInstalled(), true);
}

void DummyTest::testReviewsModel()
{
    AbstractResource* res = m_model->resourceByPackageName("Dummy 1");
    QVERIFY(res);

    ReviewsModel m;
    m.setResource(res);
    m.fetchMore();

    QVERIFY(m.rowCount()>0);

    QCOMPARE(ReviewsModel::UserChoice(m.data(m.index(0,0), ReviewsModel::UsefulChoice).toInt()), ReviewsModel::None);
    m.markUseful(0, true);
    QCOMPARE(ReviewsModel::UserChoice(m.data(m.index(0,0), ReviewsModel::UsefulChoice).toInt()), ReviewsModel::Yes);
    m.markUseful(0, false);
    QCOMPARE(ReviewsModel::UserChoice(m.data(m.index(0,0), ReviewsModel::UsefulChoice).toInt()), ReviewsModel::No);
}
