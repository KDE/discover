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
#include <resources/ResourcesUpdatesModel.h>
#include <UpdateModel/UpdateModel.h>
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

QTEST_MAIN(DummyTest)

AbstractResourcesBackend* backendByName(ResourcesModel* m, const QString& name)
{
    QVector<AbstractResourcesBackend*> backends = m->backends();
    foreach(AbstractResourcesBackend* backend, backends) {
        if(QString::fromLatin1(backend->metaObject()->className()) == name) {
            return backend;
        }
    }
    return nullptr;
}

DummyTest::DummyTest(QObject* parent): QObject(parent)
{
    m_model = new ResourcesModel(QStringLiteral("dummy-backend"), this);
    new ModelTest(m_model, m_model);

    m_appBackend = backendByName(m_model, QStringLiteral("DummyBackend"));
}

void DummyTest::init()
{
    QVERIFY(m_appBackend);
    while(m_appBackend->isFetching()) {
        QSignalSpy spy(m_appBackend, &AbstractResourcesBackend::fetchingChanged);
        QVERIFY(spy.wait());
    }
}

void DummyTest::testReadData()
{
    QBENCHMARK {
        for(int i=0, c=m_model->rowCount(); i<c; i++) {
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
    pm.setSearch(QStringLiteral("techie"));
    QCOMPARE(m_appBackend->property("startElements").toInt(), pm.rowCount());
    pm.setSearch(QString());
    QCOMPARE(m_appBackend->property("startElements").toInt()*2, pm.rowCount());
}

void DummyTest::testFetch()
{
    QCOMPARE(m_appBackend->property("startElements").toInt()*2, m_model->rowCount());

    //fetches updates, adds new things
    m_appBackend->messageActions().at(0)->trigger();
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
        pm.setDynamicSortFilter(true);
        pm.sort(0);
        QCOMPARE(pm.sortColumn(), 0);
        QCOMPARE(pm.sortOrder(), Qt::AscendingOrder);
        QString last;
        for(int i = 0, count = pm.rowCount(); i<count; ++i) {
            const QString current = pm.index(i, 0).data(pm.sortRole()).toString();
            if (!last.isEmpty()) {
                QCOMPARE(c.compare(last, current), -1);
            }
            last = current;
        }
    }

    QBENCHMARK_ONCE {
        pm.setSortRole(ResourcesModel::SortableRatingRole);
        int last=-1;
        for(int i = 0, count = pm.rowCount(); i<count; ++i) {
            const int current = pm.index(i, 0).data(pm.sortRole()).toInt();
            QVERIFY(last<=current);
            last = current;
        }
    }
}

void DummyTest::testInstallAddons()
{
    AbstractResource* res = m_model->resourceByPackageName(QStringLiteral("Dummy 1"));
    QVERIFY(res);

    ApplicationAddonsModel m;
    m.setApplication(res);
    QCOMPARE(m.rowCount(), res->addonsInformation().count());
    QCOMPARE(res->addonsInformation().at(0).isInstalled(), false);

    QString firstAddonName = m.data(m.index(0,0)).toString();
    m.changeState(firstAddonName, true);
    QVERIFY(m.hasChanges());

    m.applyChanges();
    QSignalSpy sR(TransactionModel::global(), SIGNAL(transactionRemoved(Transaction* )));
    QVERIFY(sR.wait());
    QVERIFY(!m.hasChanges());

    QCOMPARE(m.data(m.index(0,0)).toString(), firstAddonName);
    QCOMPARE(res->addonsInformation().at(0).name(), firstAddonName);
    QCOMPARE(res->addonsInformation().at(0).isInstalled(), true);
}

void DummyTest::testReviewsModel()
{
    AbstractResource* res = m_model->resourceByPackageName(QStringLiteral("Dummy 1"));
    QVERIFY(res);

    ReviewsModel m;
    new ModelTest(&m, &m);
    m.setResource(res);
    m.fetchMore();

    QVERIFY(m.rowCount()>0);

    QCOMPARE(ReviewsModel::UserChoice(m.data(m.index(0,0), ReviewsModel::UsefulChoice).toInt()), ReviewsModel::None);
    m.markUseful(0, true);
    QCOMPARE(ReviewsModel::UserChoice(m.data(m.index(0,0), ReviewsModel::UsefulChoice).toInt()), ReviewsModel::Yes);
    m.markUseful(0, false);
    QCOMPARE(ReviewsModel::UserChoice(m.data(m.index(0,0), ReviewsModel::UsefulChoice).toInt()), ReviewsModel::No);

    res = m_model->resourceByPackageName(QStringLiteral("Dummy 2"));
    m.setResource(res);
    m.fetchMore();

    QSignalSpy spy(&m, &ReviewsModel::rowsChanged);
    QVERIFY(m.rowCount()>0);
}

void DummyTest::testUpdateModel()
{
    ResourcesUpdatesModel ruModel;
    new ModelTest(&ruModel, &ruModel);
    UpdateModel model;
    new ModelTest(&model, &model);
    model.setBackend(&ruModel);

    QCOMPARE(model.rowCount(), 2);
    QCOMPARE(model.hasUpdates(), true);
}
