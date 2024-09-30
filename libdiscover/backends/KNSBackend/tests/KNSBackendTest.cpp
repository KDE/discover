/*
 *   SPDX-FileCopyrightText: 2012 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "KNSBackendTest.h"
#include "utils.h"
#include <Category/Category.h>
#include <Category/CategoryModel.h>
#include <DiscoverBackendsFactory.h>
#include <KNSBackend.h>
#include <QStandardPaths>
#include <ReviewsBackend/AbstractReviewsBackend.h>
#include <ReviewsBackend/Rating.h>
#include <ReviewsBackend/Review.h>
#include <resources/AbstractBackendUpdater.h>
#include <resources/AbstractResource.h>
#include <resources/ResourcesModel.h>

#include <QSignalSpy>
#include <QTest>

QTEST_MAIN(KNSBackendTest)

static QString s_knsrcName = QStringLiteral("ksplash.knsrc");

KNSBackendTest::KNSBackendTest(QObject *parent)
    : QObject(parent)
    , m_resource(nullptr)
{
    QStandardPaths::setTestModeEnabled(true);
    QDir(QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation)).removeRecursively();

    ResourcesModel *model = new ResourcesModel(QStringLiteral("kns-backend"), this);
    Q_ASSERT(!model->backends().isEmpty());
    auto findTestBackend = [](AbstractResourcesBackend *backend) {
        return backend->name() == QLatin1String("ksplash.knsrc");
    };
    m_backend = kFilter<QVector<AbstractResourcesBackend *>>(model->backends(), findTestBackend).value(0);

    if (!m_backend || !m_backend->isValid()) {
        qWarning() << "couldn't run the test";
        exit(0);
    }

    connect(m_backend->reviewsBackend(), &AbstractReviewsBackend::reviewsReady, this, &KNSBackendTest::reviewsArrived);
}

QVector<AbstractResource *> KNSBackendTest::getResources(ResultsStream *stream, bool canBeEmpty)
{
    Q_ASSERT(stream);
    Q_ASSERT(stream->objectName() != QLatin1String("KNS-void"));
    QSignalSpy spyResources(stream, &ResultsStream::destroyed);
    QVector<AbstractResource *> resources;
    connect(stream, &ResultsStream::resourcesFound, this, [&resources, stream](const QVector<StreamResult> &results) {
        resources += kTransform<QVector<AbstractResource *>>(results, [](auto result) {
            return result.resource;
        });
        Q_EMIT stream->fetchMore();
    });
    bool waited = spyResources.wait(10000);
    if (!waited) {
        if (auto x = qobject_cast<AggregatedResultsStream *>(stream)) {
            qDebug() << "waited" << x->streams();
        }
    }
    Q_ASSERT(waited);
    Q_ASSERT(!resources.isEmpty() || canBeEmpty);
    return resources;
}

QVector<AbstractResource *> KNSBackendTest::getAllResources(AbstractResourcesBackend *backend)
{
    AbstractResourcesBackend::Filters filters;
    if (CategoryModel::global()->rootCategories().isEmpty()) {
        CategoryModel::global()->populateCategories();
    }
    filters.category = CategoryModel::global()->rootCategories().constFirst();
    return getResources(backend->search(filters));
}

void KNSBackendTest::testRetrieval()
{
    QVERIFY(m_backend->backendUpdater());
    QCOMPARE(m_backend->updatesCount(), m_backend->backendUpdater()->toUpdate().count());

    QSignalSpy spy(m_backend, &AbstractResourcesBackend::fetchingChanged);
    QVERIFY(!m_backend->isFetching() || spy.wait());

    const auto resources = getAllResources(m_backend);
    for (auto ressource : resources) {
        QVERIFY(!ressource->name().isEmpty());
        QVERIFY(!ressource->origin().isEmpty());
        QVERIFY(!ressource->icon().isNull());
        //     QVERIFY(!ressource->comment().isEmpty());
        //     QVERIFY(!ressource->longDescription().isEmpty());
        //     QVERIFY(!ressource->license().isEmpty());
        QVERIFY(ressource->homepage().isValid() && !ressource->homepage().isEmpty());
        QVERIFY(ressource->state() > AbstractResource::Broken);
        QVERIFY(ressource->addonsInformation().isEmpty());

        QSignalSpy spy(ressource, &AbstractResource::screenshotsFetched);
        ressource->fetchScreenshots();
        QVERIFY(spy.count() || spy.wait());

        QSignalSpy spy1(ressource, &AbstractResource::changelogFetched);
        ressource->fetchChangelog();
        QVERIFY(spy1.count() || spy1.wait());
    }
}

void KNSBackendTest::testReviews()
{
    const auto resources = getAllResources(m_backend);
    auto reviewsBackend = m_backend->reviewsBackend();
    QVERIFY(!reviewsBackend->hasCredentials());
    for (auto resource : resources) {
        auto rating = reviewsBackend->ratingForApplication(resource);
        QCOMPARE(rating.packageName(), resource->packageName());
        QVERIFY(rating.rating() > 0 && rating.rating() <= 10);
    }

    auto resource = resources.first();
    QSignalSpy spy(reviewsBackend, &AbstractReviewsBackend::reviewsReady);
    reviewsBackend->fetchReviews(resource);
    QVERIFY(spy.count() || spy.wait());
}

void KNSBackendTest::reviewsArrived(AbstractResource *resource, const QVector<ReviewPtr> &reviews)
{
    m_resource = resource;
    m_reviews = reviews;
}

void KNSBackendTest::testResourceByUrl()
{
    AbstractResourcesBackend::Filters filters;
    filters.resourceUrl = QUrl(QLatin1String("kns://") + m_backend->name() + QLatin1String("/api.kde-look.org/1136471"));
    const QVector<AbstractResource *> resources = getResources(m_backend->search(filters));
    const QVector<QUrl> urls = kTransform<QVector<QUrl>>(resources, [](AbstractResource *resource) {
        return resource->url();
    });
    QCOMPARE(urls.count(), 1);
    QCOMPARE(filters.resourceUrl, urls.constFirst());

    auto resource = resources.constFirst();
    QVERIFY(!resource->isInstalled()); // Make sure .qttest is clean before running the test

    QSignalSpy spy(resource, &AbstractResource::stateChanged);
    auto backend = resource->backend();
    backend->installApplication(resource);
    QVERIFY(spy.wait());
    backend->removeApplication(resource);
    QVERIFY(spy.wait());
    QCOMPARE(spy.count(), 2);
    QVERIFY(!resource->isInstalled());
}

void KNSBackendTest::testResourceByUrlResourcesModel()
{
    AbstractResourcesBackend::Filters filter;
    filter.resourceUrl = QUrl(QStringLiteral("kns://plasmoids.knsrc/store.kde.org/1169537")); // Wrong domain

    auto resources = getResources(ResourcesModel::global()->search(filter), true);
    const QVector<QUrl> urls = kTransform<QVector<QUrl>>(resources, [](AbstractResource *resource) {
        return resource->url();
    });
    QCOMPARE(urls.count(), 0);
}

#include "moc_KNSBackendTest.cpp"
