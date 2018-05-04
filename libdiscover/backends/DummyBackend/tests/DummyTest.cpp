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
#include "DiscoverBackendsFactory.h"
#include <resources/ResourcesUpdatesModel.h>
#include <UpdateModel/UpdateModel.h>
#include <tests/modeltest.h>
#include <resources/ResourcesModel.h>
#include <resources/ResourcesProxyModel.h>
#include <resources/AbstractBackendUpdater.h>
#include <ApplicationAddonsModel.h>
#include <Transaction/TransactionModel.h>
#include <ReviewsBackend/ReviewsModel.h>
#include <ScreenshotsModel.h>
#include <Category/CategoryModel.h>
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
    DiscoverBackendsFactory::setRequestedBackends({ QStringLiteral("dummy-backend") });

    m_model = new ResourcesModel(QStringLiteral("dummy-backend"), this);
    m_appBackend = backendByName(m_model, QStringLiteral("DummyBackend"));

    CategoryModel::global()->populateCategories();
}

void DummyTest::initTestCase()
{
    QVERIFY(m_appBackend);
    while(m_appBackend->isFetching()) {
        QSignalSpy spy(m_appBackend, &AbstractResourcesBackend::fetchingChanged);
        QVERIFY(spy.wait());
    }
}

QVector<AbstractResource*> fetchResources(ResultsStream* stream)
{
    QVector<AbstractResource*> ret;
    QObject::connect(stream, &ResultsStream::resourcesFound, stream, [&ret](const QVector<AbstractResource*>& res) { ret += res; });
    QSignalSpy spy(stream, &ResultsStream::destroyed);
    Q_ASSERT(spy.wait());
    return ret;
}

void DummyTest::testReadData()
{
    const auto resources = fetchResources(m_appBackend->search({}));

    QCOMPARE(m_appBackend->property("startElements").toInt()*2, resources.size());
    QBENCHMARK {
        for(AbstractResource* res: resources) {
            QVERIFY(!res->name().isEmpty());
        }
    }
}

void DummyTest::testProxy()
{
    ResourcesProxyModel pm;
    QSignalSpy spy(&pm, &ResourcesProxyModel::busyChanged);
//     QVERIFY(spy.wait());
    QVERIFY(!pm.isBusy());

    pm.setFiltersFromCategory(CategoryModel::global()->rootCategories().first());
    pm.componentComplete();
    QVERIFY(pm.isBusy());
    QVERIFY(spy.wait());
    QVERIFY(!pm.isBusy());

    QCOMPARE(m_appBackend->property("startElements").toInt()*2, pm.rowCount());
    pm.setSearch(QStringLiteral("techie"));
    QVERIFY(pm.isBusy());
    QVERIFY(spy.wait());
    QVERIFY(!pm.isBusy());
    QCOMPARE(m_appBackend->property("startElements").toInt(), pm.rowCount());
    QCOMPARE(pm.subcategories().count(), 7);
    pm.setSearch(QString());
    QVERIFY(pm.isBusy());
    QVERIFY(spy.wait());
    QVERIFY(!pm.isBusy());
    QCOMPARE(m_appBackend->property("startElements").toInt()*2, pm.rowCount());
}

void DummyTest::testProxySorting()
{
    ResourcesProxyModel pm;
    QSignalSpy spy(&pm, &ResourcesProxyModel::busyChanged);
//     QVERIFY(spy.wait());
    QVERIFY(!pm.isBusy());

    pm.setFiltersFromCategory(CategoryModel::global()->rootCategories().first());
    pm.setSortOrder(Qt::DescendingOrder);
    pm.setSortRole(ResourcesProxyModel::RatingCountRole);
    pm.componentComplete();
    QVERIFY(pm.isBusy());
    QVERIFY(spy.wait());
    QVERIFY(!pm.isBusy());

    QCOMPARE(m_appBackend->property("startElements").toInt()*2, pm.rowCount());
    QVariant lastRatingCount;
    for(int i=0, rc=pm.rowCount(); i<rc; ++i) {
        const QModelIndex mi = pm.index(i, 0);

        const auto value = mi.data(pm.sortRole());
        QVERIFY(i==0 || value <= lastRatingCount);
        lastRatingCount = value;
    }
}

void DummyTest::testFetch()
{
    const auto resources = fetchResources(m_appBackend->search({}));
    QCOMPARE(m_appBackend->property("startElements").toInt()*2, resources.count());

    //fetches updates, adds new things
    m_appBackend->checkForUpdates();
    QSignalSpy spy(m_model, SIGNAL(allInitialized()));
    QVERIFY(spy.wait(80000));
    auto resources2 = fetchResources(m_appBackend->search({}));
    QCOMPARE(m_appBackend->property("startElements").toInt()*4, resources2.count());
}

void DummyTest::testSort()
{
    ResourcesProxyModel pm;

    QCollator c;
    QBENCHMARK_ONCE {
        pm.setSortRole(ResourcesProxyModel::NameRole);
        pm.sort(0);
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
        pm.setSortRole(ResourcesProxyModel::SortableRatingRole);
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
    AbstractResourcesBackend::Filters filter;
    filter.resourceUrl = QUrl(QStringLiteral("dummy://Dummy.1"));

    const auto resources = fetchResources(m_appBackend->search(filter));
    QCOMPARE(resources.count(), 1);
    AbstractResource* res = resources.first();
    QVERIFY(res);

    ApplicationAddonsModel m;
    new ModelTest(&m, &m);
    m.setApplication(res);
    QCOMPARE(m.rowCount(), res->addonsInformation().count());
    QCOMPARE(res->addonsInformation().at(0).isInstalled(), false);

    QString firstAddonName = m.data(m.index(0,0)).toString();
    m.changeState(firstAddonName, true);
    QVERIFY(m.hasChanges());

    m.applyChanges();
    QSignalSpy sR(TransactionModel::global(), &TransactionModel::transactionRemoved);
    QVERIFY(sR.wait());
    QVERIFY(!m.hasChanges());

    QCOMPARE(m.data(m.index(0,0)).toString(), firstAddonName);
    QCOMPARE(res->addonsInformation().at(0).name(), firstAddonName);
    QCOMPARE(res->addonsInformation().at(0).isInstalled(), true);

    m.changeState(m.data(m.index(1,0)).toString(), true);
    QVERIFY(m.hasChanges());
    for(int i=0, c=m.rowCount(); i<c; ++i) {
        const auto idx = m.index(i, 0);
        QCOMPARE(idx.data(Qt::CheckStateRole).toInt(), int(i<=1 ? Qt::Checked : Qt::Unchecked));
        QVERIFY(!idx.data(ApplicationAddonsModel::PackageNameRole).toString().isEmpty());
    }
    m.discardChanges();
    QVERIFY(!m.hasChanges());
}

void DummyTest::testReviewsModel()
{
    AbstractResourcesBackend::Filters filter;
    filter.resourceUrl = QUrl(QStringLiteral("dummy://Dummy.1"));

    const auto resources = fetchResources(m_appBackend->search(filter));
    QCOMPARE(resources.count(), 1);
    AbstractResource* res = resources.first();
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

    const auto resources2 = fetchResources(m_appBackend->search(filter));
    QCOMPARE(resources2.count(), 1);
    res = resources2.first();
    m.setResource(res);
    m.fetchMore();

    QSignalSpy spy(&m, &ReviewsModel::rowsChanged);
    QVERIFY(m.rowCount()>0);
}

void DummyTest::testUpdateModel()
{
    const auto backend = m_model->backends().first();

    ResourcesUpdatesModel ruModel;
    new ModelTest(&ruModel, &ruModel);
    UpdateModel model;
    new ModelTest(&model, &model);
    model.setBackend(&ruModel);

    QCOMPARE(model.rowCount(), 4*backend->property("startElements").toInt()/3);
    QCOMPARE(model.hasUpdates(), true);
}

void DummyTest::testScreenshotsModel()
{
    AbstractResourcesBackend::Filters filter;
    filter.resourceUrl = QUrl(QStringLiteral("dummy://Dummy.1"));

    ScreenshotsModel m;
    new ModelTest(&m, &m);

    const auto resources = fetchResources(m_appBackend->search(filter));
    QCOMPARE(resources.count(), 1);
    AbstractResource* res = resources.first();
    QVERIFY(res);
    m.setResource(res);
    QCOMPARE(res, m.resource());

    int c=m.rowCount();
    for(int i=0; i<c; ++i) {
        const auto idx = m.index(i, 0);
        QVERIFY(!idx.data(ScreenshotsModel::ThumbnailUrl).isNull());
        QVERIFY(!idx.data(ScreenshotsModel::ScreenshotUrl).isNull());
    }
}

//TODO test cancel transaction
