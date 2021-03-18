/*
 *   SPDX-FileCopyrightText: 2012 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "DummyTest.h"
#include <ApplicationAddonsModel.h>
#include <KFormat>
#include <QAbstractItemModelTester>
#include <ReviewsBackend/ReviewsModel.h>
#include <Transaction/TransactionModel.h>
#include <UpdateModel/UpdateModel.h>
#include <resources/AbstractBackendUpdater.h>
#include <resources/ResourcesModel.h>
#include <resources/ResourcesProxyModel.h>
#include <resources/ResourcesUpdatesModel.h>

#include <QTest>
#include <QtTest>

class UpdateDummyTest : public QObject
{
    Q_OBJECT
public:
    AbstractResourcesBackend *backendByName(ResourcesModel *m, const QString &name)
    {
        QVector<AbstractResourcesBackend *> backends = m->backends();
        foreach (AbstractResourcesBackend *backend, backends) {
            if (QLatin1String(backend->metaObject()->className()) == name) {
                return backend;
            }
        }
        return nullptr;
    }

    UpdateDummyTest(QObject *parent = nullptr)
        : QObject(parent)
    {
        m_model = new ResourcesModel(QStringLiteral("dummy-backend"), this);
        m_appBackend = backendByName(m_model, QStringLiteral("DummyBackend"));
    }

private Q_SLOTS:
    void init()
    {
        QVERIFY(m_appBackend);
        while (m_appBackend->isFetching()) {
            QSignalSpy spy(m_appBackend, &AbstractResourcesBackend::fetchingChanged);
            QVERIFY(spy.wait());
        }
    }

    void testInformation()
    {
        ResourcesUpdatesModel *rum = new ResourcesUpdatesModel(this);
        new QAbstractItemModelTester(rum, rum);

        UpdateModel *m = new UpdateModel(this);
        new QAbstractItemModelTester(m, m);
        m->setBackend(rum);

        rum->prepare();
        QSignalSpy spySetup(m_appBackend->backendUpdater(), &AbstractBackendUpdater::progressingChanged);
        QVERIFY(!m_appBackend->backendUpdater()->isProgressing() || spySetup.wait());
        QCOMPARE(m_appBackend->updatesCount(), m_appBackend->property("startElements").toInt());
        QCOMPARE(m->hasUpdates(), true);

        QCOMPARE(m->index(0, 0).data(UpdateModel::ChangelogRole).toString(), QString{});

        QSignalSpy spy(m, &QAbstractItemModel::dataChanged);
        m->fetchUpdateDetails(0);
        QVERIFY(spy.count() || spy.wait());
        QCOMPARE(spy.count(), 1);
        delete m;
    }

    void testUpdate()
    {
        ResourcesUpdatesModel *rum = new ResourcesUpdatesModel(this);
        new QAbstractItemModelTester(rum, rum);

        UpdateModel *m = new UpdateModel(this);
        new QAbstractItemModelTester(m, m);
        m->setBackend(rum);

        rum->prepare();
        QSignalSpy spySetup(m_appBackend->backendUpdater(), &AbstractBackendUpdater::progressingChanged);
        QVERIFY(!m_appBackend->backendUpdater()->isProgressing() || spySetup.wait());
        QCOMPARE(m_appBackend->updatesCount(), m_appBackend->property("startElements").toInt());
        QCOMPARE(m->hasUpdates(), true);

        for (int i = 0, c = m->rowCount(); i < c; ++i) {
            const QModelIndex resourceIdx = m->index(i, 0);
            QVERIFY(resourceIdx.isValid());

            AbstractResource *res = qobject_cast<AbstractResource *>(resourceIdx.data(UpdateModel::ResourceRole).value<QObject *>());
            QVERIFY(res);

            QCOMPARE(Qt::CheckState(resourceIdx.data(Qt::CheckStateRole).toInt()), Qt::Checked);
            QVERIFY(m->setData(resourceIdx, int(Qt::Unchecked), Qt::CheckStateRole));
            QCOMPARE(Qt::CheckState(resourceIdx.data(Qt::CheckStateRole).toInt()), Qt::Unchecked);
            QCOMPARE(resourceIdx.data(Qt::DisplayRole).toString(), res->name());

            if (i != 0) {
                QVERIFY(m->setData(resourceIdx, int(Qt::Checked), Qt::CheckStateRole));
            }
        }

        QSignalSpy spy(rum, &ResourcesUpdatesModel::progressingChanged);
        QVERIFY(!rum->isProgressing() || spy.wait());
        QCOMPARE(rum->isProgressing(), false);

        QCOMPARE(m_appBackend->updatesCount(), m->rowCount());
        QCOMPARE(m->hasUpdates(), true);

        rum->prepare();

        spy.clear();
        QCOMPARE(rum->isProgressing(), false);
        rum->updateAll();
        QVERIFY(spy.count() || spy.wait());
        QCOMPARE(rum->isProgressing(), true);

        QCOMPARE(TransactionModel::global()->rowCount(), 1);
        connect(TransactionModel::global(), &TransactionModel::progressChanged, this, []() {
            const int progress = TransactionModel::global()->progress();
            static int lastProgress = -1;
            Q_ASSERT(progress >= lastProgress || (TransactionModel::global()->rowCount() == 0 && progress == 0));
            lastProgress = progress;
        });

        QTest::qWait(20);
        QScopedPointer<ResourcesUpdatesModel> rum2(new ResourcesUpdatesModel(this));
        new QAbstractItemModelTester(rum2.data(), rum2.data());

        QScopedPointer<UpdateModel> m2(new UpdateModel(this));
        new QAbstractItemModelTester(m2.data(), m2.data());
        m->setBackend(rum2.data());

        QCOMPARE(rum->isProgressing(), true);
        QVERIFY(spy.wait());
        QCOMPARE(rum->isProgressing(), false);

        QCOMPARE(m_appBackend->updatesCount(), 0);
        QCOMPARE(m->hasUpdates(), false);
    }

private:
    ResourcesModel *m_model;
    AbstractResourcesBackend *m_appBackend;
};

QTEST_MAIN(UpdateDummyTest)

#include "UpdateDummyTest.moc"
