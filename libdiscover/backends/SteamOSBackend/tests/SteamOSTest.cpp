/***************************************************************************
 *   SPDX-FileCopyrightText: 2024 Jeremy Whiting <jpwhiting@kde.org>       *
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

#include "libdiscover_steamos_debug.h"

#include "atomupd1_adaptor.h"
#include "mock_server.h"

#include "SteamOSTest.h"

AbstractResourcesBackend *SteamOSTest::backendByName(ResourcesModel *m, const QString &name)
{
    const QVector<AbstractResourcesBackend *> backends = m->backends();
    for (AbstractResourcesBackend *backend : backends) {
        if (QLatin1String(backend->metaObject()->className()) == name) {
            return backend;
        }
    }
    return nullptr;
}

SteamOSTest::SteamOSTest(QObject *parent)
    : QObject(parent)
{
    QDir(QStandardPaths::writableLocation(QStandardPaths::TempLocation) + QLatin1String("/discover-steamos-test")).removeRecursively();

    m_server = new MockServer();
    m_adaptor = new Atomupd1Adaptor(m_server);
    QDBusConnection dbus = QDBusConnection::sessionBus();
    dbus.registerObject(QLatin1String("/com/steampowered/Atomupd1"), m_server);
    dbus.registerService(QLatin1String("com.steampowered.Atomupd1"));
    QStandardPaths::setTestModeEnabled(true);
    qputenv("STEAMOS_TEST_MODE", "ON");
    m_model = new ResourcesModel(QStringLiteral("steamos-backend"), this);
    m_appBackend = backendByName(m_model, QStringLiteral("SteamOSBackend"));
}

void SteamOSTest::initTestCase()
{
    QVERIFY(m_appBackend);
    if (!m_appBackend || !m_appBackend->isValid()) {
        qWarning() << "couldn't run the test";
        exit(0);
    }

    // Check both MockServer and dbus interface to Version are working
    Q_ASSERT(m_server->currentVersion() == TEST_VERSION);
    Q_ASSERT(m_adaptor->currentVersion() == TEST_VERSION);

    // Check buildid also
    Q_ASSERT(m_server->currentBuildID() == TEST_BUILDID);
    Q_ASSERT(m_adaptor->currentBuildID() == TEST_BUILDID);

    // Check branch next
    Q_ASSERT(m_server->branch() == TEST_BRANCH);
    Q_ASSERT(m_adaptor->branch() == TEST_BRANCH);
}

QTEST_GUILESS_MAIN(SteamOSTest)
