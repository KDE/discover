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
#include <UpdateModel/UpdateModel.h>
#include <resources/ResourcesUpdatesModel.h>

#include <qtest.h>
#include <QtTest>
#include <QAction>

class UpdateDummyTest
    : public QObject
{
    Q_OBJECT
public:
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

    UpdateDummyTest(QObject* parent = nullptr): QObject(parent)
    {
        m_model = new ResourcesModel("muon-dummy-backend", this);
//         new ModelTest(m_model, m_model);

        m_appBackend = backendByName(m_model, "DummyBackend");
        QVERIFY(m_appBackend);
        QSignalSpy spy(m_appBackend, SIGNAL(backendReady()));
        QVERIFY(spy.wait(0));
    }

private slots:
    void testUpdate()
    {
        ResourcesUpdatesModel* rum = new ResourcesUpdatesModel(this);
//         new ModelTest(rum, rum);

        UpdateModel* m = new UpdateModel(this);
//         new ModelTest(m, m);
        m->setBackend(rum);

        rum->prepare();
        QCOMPARE(m_appBackend->updatesCount(), 212);
        QCOMPARE(m->hasUpdates(), true);

        QCOMPARE(m->index(0,0).child(0,0).data(Qt::CheckStateRole).toBool(), true);
        m->setData(m->index(0,0).child(0,0), false, Qt::CheckStateRole);
        QCOMPARE(m->index(0,0).child(0,0).data(Qt::CheckStateRole).toBool(), false);


        rum->updateAll();

        QSignalSpy spy(rum, SIGNAL(progressingChanged()));
        QVERIFY(spy.wait());
        QCOMPARE(rum->isProgressing(), true);
        QVERIFY(spy.wait());
        QCOMPARE(rum->isProgressing(), false);

        QCOMPARE(m_appBackend->updatesCount(), 1);
        QCOMPARE(m->hasUpdates(), true);

        rum->prepare();
        rum->updateAll();
        QVERIFY(spy.wait());
        QCOMPARE(rum->isProgressing(), true);
        QVERIFY(spy.wait());
        QCOMPARE(rum->isProgressing(), false);

        QCOMPARE(m_appBackend->updatesCount(), 0);
        QCOMPARE(m->hasUpdates(), false);
    }

private:
    ResourcesModel* m_model;
    AbstractResourcesBackend* m_appBackend;
};

QTEST_MAIN(UpdateDummyTest);

#include "UpdateDummyTest.moc"
