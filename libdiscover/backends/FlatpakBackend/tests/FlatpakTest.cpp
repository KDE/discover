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

#include <QSignalSpy>
#include <QStandardPaths>
#include <QTest>

class FlatpakTest : public QObject
{
    Q_OBJECT
public:
    AbstractResourcesBackend *backendByName(ResourcesModel *m, const QString &name)
    {
        const QList<AbstractResourcesBackend *> backends = m->backends();
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
        QDir(QStandardPaths::writableLocation(QStandardPaths::TempLocation) + QLatin1String("/discover-flatpak-test")).removeRecursively();

        QStandardPaths::setTestModeEnabled(true);
        qputenv("FLATPAK_TEST_MODE", "ON");
        m_model = new ResourcesModel(QStringLiteral("flatpak-backend"), this);
        m_appBackend = backendByName(m_model, QStringLiteral("FlatpakBackend"));
    }

private Q_SLOTS:
    void initTestCase()
    {
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
        f.resourceUrl = QUrl(QStringLiteral("appstream://dosbox.desktop"));
        const auto res = getResources(m_appBackend->search(f));
        QCOMPARE(res.count(), 1);

        const auto ourResource = res.constFirst();
        QCOMPARE(ourResource->state(), AbstractResource::None);
        QCOMPARE(waitTransaction(m_appBackend->installApplication(ourResource)), Transaction::DoneStatus);
        QCOMPARE(ourResource->state(), AbstractResource::Installed);
        QCOMPARE(waitTransaction(m_appBackend->removeApplication(ourResource)), Transaction::DoneStatus);
        QCOMPARE(ourResource->state(), AbstractResource::None);
    }

    void testFlatpakref()
    {
        AbstractResourcesBackend::Filters f;
        f.resourceUrl = QUrl(QStringLiteral("https://dl.flathub.org/repo/appstream/com.dosbox.DOSBox.flatpakref"));
        const auto res = getResources(m_appBackend->search(f));
        QCOMPARE(res.count(), 1);

        f.resourceUrl = QUrl(QStringLiteral("appstream://dosbox.desktop"));
        const auto res2 = getResources(m_appBackend->search(f));
        QCOMPARE(res2, res);

        f.resourceUrl = QUrl(QStringLiteral("appstream://com.dosbox.DOSBox.desktop"));
        const auto res3 = getResources(m_appBackend->search(f));
        QCOMPARE(res3, res);
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
        int lastProgress = -1;
        connect(t, &Transaction::progressChanged, this, [t, &lastProgress] {
            Q_ASSERT(lastProgress <= t->progress());
            lastProgress = t->progress();
        });

        TransactionModel::global()->addTransaction(t);
        QSignalSpy spyInstalled(TransactionModel::global(), &TransactionModel::transactionRemoved);
        QSignalSpy destructionSpy(t, &QObject::destroyed);

        Transaction::Status ret = t->status();
        connect(TransactionModel::global(), &TransactionModel::transactionRemoved, t, [t, &ret](Transaction *trans) {
            if (trans == t) {
                ret = trans->status();
            }
        });
        connect(t, &Transaction::statusChanged, t, [t] {
            qDebug() << "status" << t->status();
        });
        while (t && spyInstalled.count() == 0) {
            qDebug() << "waiting, currently" << ret << t->progress() << spyInstalled.count() << destructionSpy.count();
            spyInstalled.wait(1000);
        }
        Q_ASSERT(destructionSpy.count() || destructionSpy.wait());
        return ret;
    }

    QList<AbstractResource *> getResources(ResultsStream *stream, bool canBeEmpty = true)
    {
        Q_ASSERT(stream);
        QSignalSpy spyResources(stream, &ResultsStream::destroyed);
        QList<AbstractResource *> resources;
        connect(stream, &ResultsStream::resourcesFound, this, [&resources](const QList<StreamResult> &res) {
            for (auto result : res) {
                resources += result.resource;
            }
        });
        Q_ASSERT(spyResources.wait(100000));
        Q_ASSERT(!resources.isEmpty() || canBeEmpty);
        return resources;
    }

    QList<AbstractResource *> getAllResources(AbstractResourcesBackend *backend)
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

QTEST_GUILESS_MAIN(FlatpakTest)

#include "FlatpakTest.moc"
