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

#include "ResourcesModelTest.h"
#include <ApplicationBackend.h>
#include <QStringList>
#include <LibQApt/Backend>
#include <KProtocolManager>
#include <qtest_kde.h>

#include "modeltest.h"
#include <Application.h>
#include <resources/ResourcesModel.h>

QTEST_KDEMAIN_CORE( ResourcesModelTest )

ResourcesModelTest::ResourcesModelTest()
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

ResourcesModelTest::~ResourcesModelTest()
{
    delete m_appBackend;
    delete m_backend;
}

void ResourcesModelTest::testReload()
{
    ResourcesModel* model = new ResourcesModel(this);
    model->addResourcesBackend(m_appBackend);
    
    QVector<AbstractResource*> apps = m_appBackend->allResources();
    QCOMPARE(apps.count(), model->rowCount());
    
    QVector<QVariant> appNames(apps.size());
    for(int i=0; i<model->rowCount(); ++i) {
        AbstractResource* app = apps[i];
        appNames[i]=app->property("packageName");
    }
    
    m_appBackend->reload();
    m_appBackend->updatesCount();
    QCOMPARE(apps, m_appBackend->allResources() );
    
    QVERIFY(!apps.isEmpty());
    QCOMPARE(apps.count(), model->rowCount());
    
    for(int i=0; i<model->rowCount(); ++i) {
        AbstractResource* app = apps[i];
        QCOMPARE(appNames[i], app->property("packageName"));
//         QCOMPARE(model->data(model->index(i), ResourcesModel::NameRole).toString(), app->name());
    }
}
