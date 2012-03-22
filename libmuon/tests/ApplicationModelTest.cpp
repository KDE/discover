/* KDevelop CMake Support
 *
 * Copyright 2006 Matt Rogers <mattr@kde.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */

#include "ApplicationModelTest.h"
#include <ApplicationModel/ApplicationModel.h>
#include <ApplicationModel/ApplicationProxyModel.h>
#include <ApplicationBackend.h>
#include <QStringList>
#include <LibQApt/Backend>
#include <KProtocolManager>

#include "modeltest.h"
#include <Application.h>

QTEST_MAIN( ApplicationModelTest )

ApplicationModelTest::ApplicationModelTest()
{
    m_backend = new QApt::Backend;
    if (m_backend->xapianIndexNeedsUpdate()) {
        m_backend->updateXapianIndex();
    }

    if (KProtocolManager::proxyType() == KProtocolManager::ManualProxy) {
        m_backend->setWorkerProxy(KProtocolManager::proxyFor("http"));
    }
    m_backend->init();
    
    m_appBackend = new ApplicationBackend(this);
    m_appBackend->setBackend(m_backend);
}

ApplicationModelTest::~ApplicationModelTest()
{
    delete m_backend;
}

void ApplicationModelTest::testReload()
{
    ApplicationModel* model = new ApplicationModel(this);
    ApplicationProxyModel* updatesProxy = new ApplicationProxyModel(this);
    updatesProxy->setBackend(m_backend);
    updatesProxy->setSourceModel(model);
    new ModelTest(model, model);
    new ModelTest(updatesProxy, updatesProxy);
    model->setBackend(m_appBackend);
    updatesProxy->setStateFilter(QApt::Package::ToUpgrade);
    
    QVector<Application*> apps = m_appBackend->applicationList();
    QVector<QString> appNames(apps.size());
    for(int i=0; i<model->rowCount(); ++i) {
        Application* app = apps[i];
        QCOMPARE(model->data(model->index(i), ApplicationModel::NameRole).toString(), app->name());
        appNames[i]=app->packageName();
        QVERIFY(app->isValid());
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
