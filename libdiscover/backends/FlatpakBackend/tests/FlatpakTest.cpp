/***************************************************************************
 *   SPDX-FileCopyrightText: 2012 Aleix Pol Gonzalez <aleixpol@blue-systems.com>       *
 *                                                                         *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 ***************************************************************************/

#include <ApplicationAddonsModel.h>
#include <Category/CategoryModel.h>
#include <ReviewsBackend/ReviewsModel.h>
#include <Transaction/TransactionModel.h>
#include <resources/AbstractBackendUpdater.h>
#include <resources/DiscoverAction.h>
#include <resources/ResourcesModel.h>
#include <resources/ResourcesProxyModel.h>
#include <resources/SourcesModel.h>

#include <QTest>
#include <QtTest>

class FlatpakTest : public QObject
{
    Q_OBJECT
public:
    AbstractResourcesBackend *backendByName(ResourcesModel *m, const QString &name)
    {
        const QVector<AbstractResourcesBackend *> backends = m->backends();
        for (AbstractResourcesBackend *backend : backends) {
            if (QLatin1String(backend->metaObject()->className()) == name) {
                return backend;
            }
        }
        return nullptr;
    }

    FlatpakTest(QObject *parent = nullptr)
        : QObject(parent)
    {
        QStandardPaths::setTestModeEnabled(true);
        qputenv("FLATPAK_TEST_MODE", "ON");
        m_model = new ResourcesModel(QStringLiteral("flatpak-backend"), this);
        m_appBackend = backendByName(m_model, QStringLiteral("FlatpakBackend"));
    }

private Q_SLOTS:
    void initTestCase()
    {
        QDir(QStandardPaths::writableLocation(QStandardPaths::TempLocation) + QLatin1String("/discover-flatpak-test")).removeRecursively();

        QVERIFY(m_appBackend);
        while (m_appBackend->isFetching()) {
            QSignalSpy spy(m_appBackend, &AbstractResourcesBackend::fetchingChanged);
            QVERIFY(spy.wait());
        }
    }

    void testAddSource()
    {
        auto res = getAllResources(m_appBackend);
        QCOMPARE(res.count(), 0);

        auto m = SourcesModel::global();
        auto bk = qobject_cast<AbstractSourcesBackend *>(m->index(0, 0).data(SourcesModel::SourcesBackend).value<QObject *>());

        QSignalSpy initializedSpy(m_appBackend, SIGNAL(initialized()));
        if (m->rowCount() == 1) {
            QSignalSpy spy(m, &SourcesModel::rowsInserted);
            qobject_cast<DiscoverAction *>(bk->actions().constFirst().value<QObject *>())->trigger();
            QVERIFY(spy.count() || spy.wait(200000));
        }
        QVERIFY(initializedSpy.count() || initializedSpy.wait(200000));
        auto resFlathub = getAllResources(m_appBackend);
        QVERIFY(resFlathub.count() > 0);
    }

    void testListOrigin()
    {
        AbstractResourcesBackend::Filters f;
        f.origin = QStringLiteral("flathub");
        auto resources = getResources(m_appBackend->search(f), true);
        QVERIFY(resources.count() > 0);
    }

    void testInstallApp()
    {
        AbstractResourcesBackend::Filters f;
        f.resourceUrl = QUrl(QStringLiteral("appstream://com.github.rssguard.desktop"));
        const auto res = getResources(m_appBackend->search(f));
        QCOMPARE(res.count(), 1);

        const auto resRssguard = res.constFirst();
        QCOMPARE(resRssguard->state(), AbstractResource::None);
        QCOMPARE(waitTransaction(m_appBackend->installApplication(resRssguard)), Transaction::DoneStatus);
        QCOMPARE(resRssguard->state(), AbstractResource::Installed);
        QCOMPARE(waitTransaction(m_appBackend->removeApplication(resRssguard)), Transaction::DoneStatus);
        QCOMPARE(resRssguard->state(), AbstractResource::None);
    }
    /*
        void testCancelInstallation()
        {
            AbstractResourcesBackend::Filters f;
            f.resourceUrl = QUrl(QStringLiteral("appstream://com.github.rssguard.desktop"));
            const auto res = getResources(m_appBackend->search(f));
            QCOMPARE(res.count(), 1);

            const auto resRssguard = res.constFirst();
            QCOMPARE(resRssguard->state(), AbstractResource::None);
            auto t = m_appBackend->installApplication(resRssguard);
            QSignalSpy spy(t, &Transaction::statusChanged);
            QVERIFY(spy.wait());
            QCOMPARE(t->status(), Transaction::CommittingStatus);
            t->cancel();
            QVERIFY(spy.wait());
            QCOMPARE(t->status(), Transaction::CancelledStatus);
        }*/

private:
    Transaction::Status waitTransaction(Transaction *t)
    {
        TransactionModel::global()->addTransaction(t);
        QSignalSpy spyInstalled(TransactionModel::global(), &TransactionModel::transactionRemoved);
        QSignalSpy destructionSpy(t, &QObject::destroyed);

        Transaction::Status ret = t->status();
        connect(TransactionModel::global(), &TransactionModel::transactionRemoved, this, [t, &ret] {
            ret = t->status();
        });
        while (t && spyInstalled.count() == 0) {
            qDebug() << "waiting, currently" << ret << spyInstalled.count() << destructionSpy.count();
            spyInstalled.wait(100);
        }
        Q_ASSERT(destructionSpy.count() || destructionSpy.wait());
        return ret;
    }

    QVector<AbstractResource *> getResources(ResultsStream *stream, bool canBeEmpty = true)
    {
        Q_ASSERT(stream);
        QSignalSpy spyResources(stream, &ResultsStream::destroyed);
        QVector<AbstractResource *> resources;
        connect(stream, &ResultsStream::resourcesFound, this, [&resources](const QVector<AbstractResource *> &res) {
            resources += res;
        });
        Q_ASSERT(spyResources.wait(10000));
        Q_ASSERT(!resources.isEmpty() || canBeEmpty);
        return resources;
    }

    QVector<AbstractResource *> getAllResources(AbstractResourcesBackend *backend)
    {
        AbstractResourcesBackend::Filters f;
        if (CategoryModel::global()->rootCategories().isEmpty())
            CategoryModel::global()->populateCategories();
        f.category = CategoryModel::global()->rootCategories().constFirst();
        return getResources(backend->search(f), true);
    }

    ResourcesModel *m_model;
    AbstractResourcesBackend *m_appBackend;
};

QTEST_MAIN(FlatpakTest)

#include "FlatpakTest.moc"
