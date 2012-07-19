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

#include "ApplicationModelTest.h"
#include <ApplicationModel/ApplicationModel.h>
#include <ApplicationModel/ApplicationProxyModel.h>
#include <ApplicationBackend.h>
#include <QStringList>
#include <LibQApt/Backend>
#include <KProtocolManager>
#include <qtest_kde.h>

#include "modeltest.h"
#include <Application.h>

QTEST_KDEMAIN_CORE( ApplicationModelTest )

ApplicationModelTest::ApplicationModelTest()
{
    m_backend = new QApt::Backend;

    if (KProtocolManager::proxyType() == KProtocolManager::ManualProxy) {
        m_backend->setWorkerProxy(KProtocolManager::proxyFor("http"));
    }
    m_backend->init();
    if (m_backend->xapianIndexNeedsUpdate()) {
        m_backend->updateXapianIndex();
    }
    
    m_appBackend = new ApplicationBackend(this);
    m_appBackend->setBackend(m_backend);
    QTest::kWaitForSignal(m_appBackend, SIGNAL(backendReady()));
}

ApplicationModelTest::~ApplicationModelTest()
{
    delete m_appBackend;
    delete m_backend;
}

void ApplicationModelTest::testReload()
{
    ApplicationModel* model = new ApplicationModel(this);
    ApplicationProxyModel* updatesProxy = new ApplicationProxyModel(this);
    updatesProxy->setBackend(m_backend);
    updatesProxy->setSourceModel(model);
//     new ModelTest(model, model);
//     new ModelTest(updatesProxy, updatesProxy);
    model->setBackend(m_appBackend);
    updatesProxy->setStateFilter(QApt::Package::ToUpgrade);
    
    QVector<Application*> apps = m_appBackend->applicationList();
    QVector<QString> appNames(apps.size());
    for(int i=0; i<model->rowCount(); ++i) {
        Application* app = apps[i];
        QCOMPARE(model->data(model->index(i), ApplicationModel::NameRole).toString(), app->name());
        appNames[i]=app->packageName();
        QVERIFY(app->isValid());
        QVERIFY(m_backend->package(app->packageName()));
//         app->fetchScreenshots();
    }
    
    m_appBackend->reload();
    m_appBackend->updatesCount();
    QCOMPARE(apps, m_appBackend->applicationList() );
    
    QVERIFY(!apps.isEmpty());
    QCOMPARE(apps.count(), model->rowCount());
    
    for(int i=0; i<model->rowCount(); ++i) {
        Application* app = apps[i];
//         qDebug() << "a" << appNames[i];
//         if(!app->package()) qDebug() << "laaaaaa" << app->packageName();
        QCOMPARE(appNames[i], app->packageName());
        QCOMPARE(model->data(model->index(i), ApplicationModel::NameRole).toString(), app->name());
//         if(appNames[i]!=app->name()) qDebug() << "ffffffff" << app->packageName() << appNames[i] << app->name() << app->isTechnical();
    }
}

void ApplicationModelTest::testSearch()
{
    ApplicationModel* model = new ApplicationModel(this);
    ApplicationProxyModel* updatesProxy = new ApplicationProxyModel(this);
    updatesProxy->setBackend(m_backend);
    updatesProxy->setSourceModel(model);
    new ModelTest(model, model);
    new ModelTest(updatesProxy, updatesProxy);
    model->setBackend(m_appBackend);
    
    updatesProxy->search("kal");
    int oldCount = updatesProxy->rowCount();
    updatesProxy->search("kalgebra");
    QVERIFY(oldCount>updatesProxy->rowCount());
}
