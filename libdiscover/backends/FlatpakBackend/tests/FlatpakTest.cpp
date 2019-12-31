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

#include <resources/ResourcesModel.h>
#include <resources/ResourcesProxyModel.h>
#include <resources/AbstractBackendUpdater.h>
#include <resources/SourcesModel.h>
#include <ApplicationAddonsModel.h>
#include <ReviewsBackend/ReviewsModel.h>
#include <Transaction/TransactionModel.h>
#include <Category/CategoryModel.h>

#include <qtest.h>
#include <QtTest>
#include <QAction>

class FlatpakTest
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

    FlatpakTest(QObject* parent = nullptr): QObject(parent)
    {
        QStandardPaths::setTestModeEnabled(true);
        qputenv("FLATPAK_TEST_MODE", "ON");
        m_model = new ResourcesModel(QStringLiteral("flatpak-backend"), this);
        m_appBackend = backendByName(m_model, QStringLiteral("FlatpakBackend"));
    }

private Q_SLOTS:
    void init()
    {
        QDir(QStandardPaths::writableLocation(QStandardPaths::TempLocation) + QLatin1String("/discover-flatpak-test")).removeRecursively();

        QVERIFY(m_appBackend);
        while(m_appBackend->isFetching()) {
            QSignalSpy spy(m_appBackend, &AbstractResourcesBackend::fetchingChanged);
            QVERIFY(spy.wait());
        }
    }

    void testAddSource()
    {
        auto res = getAllResources(m_appBackend);
        QCOMPARE(res.count(), 0);

        auto m = SourcesModel::global();
        auto bk = qobject_cast<AbstractSourcesBackend*>(m->index(0, 0).data(SourcesModel::SourcesBackend).value<QObject*>());

        QSignalSpy initializedSpy(m_appBackend, SIGNAL(initialized()));
        if (m->rowCount() == 1) {
            QSignalSpy spy(m, &SourcesModel::rowsInserted);
            qobject_cast<QAction*>(bk->actions().constFirst().value<QObject*>())->trigger();
            QVERIFY(spy.count() || spy.wait(20000));
        }
        QVERIFY(initializedSpy.count() || initializedSpy.wait(20000));
        auto resFlathub = getAllResources(m_appBackend);
        QVERIFY(resFlathub.count() > 0);
    }

    void testListOrigin()
    {
        AbstractResourcesBackend::Filters f;
        f.origin = QStringLiteral("flathub");
        auto resources= getResources(m_appBackend->search(f), true);
        QVERIFY(resources.count()>0);
    }

    void testInstallApp()
    {
        AbstractResourcesBackend::Filters f;
        f.resourceUrl = QUrl(QStringLiteral("appstream://com.github.rssguard.desktop"));
        const auto res = getResources(m_appBackend->search(f));
        QVERIFY(res.count() == 1);

        const auto resRssguard = res.constFirst();
        QCOMPARE(resRssguard->state(), AbstractResource::None);
        QCOMPARE(waitTransaction(m_appBackend->installApplication(resRssguard)), Transaction::DoneStatus);
        QCOMPARE(resRssguard->state(), AbstractResource::Installed);
        QCOMPARE(waitTransaction(m_appBackend->removeApplication(resRssguard)), Transaction::DoneStatus);
        QCOMPARE(resRssguard->state(), AbstractResource::None);
    }

    void testCancelInstallation()
    {
        AbstractResourcesBackend::Filters f;
        f.resourceUrl = QUrl(QStringLiteral("appstream://com.github.rssguard.desktop"));
        const auto res = getResources(m_appBackend->search(f));
        QVERIFY(res.count() == 1);

        const auto resRssguard = res.constFirst();
        QCOMPARE(resRssguard->state(), AbstractResource::None);
        auto t = m_appBackend->installApplication(resRssguard);
        QSignalSpy spy(t, &Transaction::statusChanged);
        QVERIFY(spy.wait());
        QCOMPARE(t->status(), Transaction::CommittingStatus);
        t->cancel();
        QCOMPARE(t->status(), Transaction::CancelledStatus);
    }

private:
    Transaction::Status waitTransaction(Transaction* t) {
        TransactionModel::global()->addTransaction(t);
        QSignalSpy spyInstalled(TransactionModel::global(), &TransactionModel::transactionRemoved);
        QSignalSpy destructionSpy(t, &QObject::destroyed);

        Transaction::Status ret = t->status();
        connect(TransactionModel::global(), &TransactionModel::transactionRemoved, this, [t, &ret] { ret = t->status(); });
        while (t && spyInstalled.count() == 0) {
            qDebug() << "waiting, currently" << ret << spyInstalled.count() << destructionSpy.count();
            spyInstalled.wait(100);
        }
        Q_ASSERT(destructionSpy.count() || destructionSpy.wait());
        return ret;
    }

    QVector<AbstractResource*> getResources(ResultsStream* stream, bool canBeEmpty = true)
    {
        Q_ASSERT(stream);
        QSignalSpy spyResources(stream, &ResultsStream::destroyed);
        QVector<AbstractResource*> resources;
        connect(stream, &ResultsStream::resourcesFound, this, [&resources](const QVector<AbstractResource*>& res) { resources += res; });
        Q_ASSERT(spyResources.wait(10000));
        Q_ASSERT(!resources.isEmpty() || canBeEmpty);
        return resources;
    }

    QVector<AbstractResource*> getAllResources(AbstractResourcesBackend* backend)
    {
        AbstractResourcesBackend::Filters f;
        if (CategoryModel::global()->rootCategories().isEmpty())
            CategoryModel::global()->populateCategories();
        f.category = CategoryModel::global()->rootCategories().constFirst();
        return getResources(backend->search(f), true);
    }

    ResourcesModel* m_model;
    AbstractResourcesBackend* m_appBackend;
};

QTEST_MAIN(FlatpakTest)

#include "FlatpakTest.moc"
