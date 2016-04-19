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
#include <tests/modeltest.h>
#include <KFormat>
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
            if(QLatin1String(backend->metaObject()->className()) == name) {
                return backend;
            }
        }
        return nullptr;
    }

    UpdateDummyTest(QObject* parent = nullptr): QObject(parent)
    {
        m_model = new ResourcesModel(QStringLiteral("dummy-backend"), this);
        new ModelTest(m_model, m_model);

        m_appBackend = backendByName(m_model, QStringLiteral("DummyBackend"));
    }

private Q_SLOTS:
    void init()
    {
        QVERIFY(m_appBackend);
        while(m_appBackend->isFetching()) {
            QSignalSpy spy(m_appBackend, &AbstractResourcesBackend::fetchingChanged);
            QVERIFY(spy.wait());
        }
    }

    void testUpdate()
    {
        ResourcesUpdatesModel* rum = new ResourcesUpdatesModel(this);
        new ModelTest(rum, rum);

        UpdateModel* m = new UpdateModel(this);
        new ModelTest(m, m);
        m->setBackend(rum);

        rum->prepare();
        QCOMPARE(m_appBackend->updatesCount(), m_appBackend->property("startElements").toInt()*2/3-1);
        QCOMPARE(m->hasUpdates(), true);

        for(int i=0, c=m->rowCount(); i<c; ++i) {
            const QModelIndex resourceIdx = m->index(i,0);
            QVERIFY(resourceIdx.isValid());

            AbstractResource* res = qobject_cast<AbstractResource*>(resourceIdx.data(UpdateModel::ResourceRole).value<QObject*>());
            QVERIFY(res);
            QCOMPARE(resourceIdx.data(UpdateModel::SizeRole).toString(), QStringLiteral("123 B"));

            QCOMPARE(Qt::CheckState(resourceIdx.data(Qt::CheckStateRole).toInt()), Qt::Checked);
            QVERIFY(m->setData(resourceIdx, int(Qt::Unchecked), Qt::CheckStateRole));
            QCOMPARE(Qt::CheckState(resourceIdx.data(Qt::CheckStateRole).toInt()), Qt::Unchecked);
            QCOMPARE(resourceIdx.data(Qt::DisplayRole).toString(), res->name());

            if (i!=0) {
                QVERIFY(m->setData(resourceIdx, int(Qt::Checked), Qt::CheckStateRole));
            }
        }


        rum->updateAll();

        QSignalSpy spy(rum, &ResourcesUpdatesModel::progressingChanged);
        QVERIFY(spy.wait());
        QCOMPARE(rum->isProgressing(), true);
        QVERIFY(spy.wait());
        QCOMPARE(rum->isProgressing(), false);

        QCOMPARE(m_appBackend->updatesCount(), m->rowCount());
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

QTEST_MAIN(UpdateDummyTest)

#include "UpdateDummyTest.moc"
