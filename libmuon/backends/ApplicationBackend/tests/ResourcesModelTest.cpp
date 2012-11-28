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
#include <QStringList>
#include <KProtocolManager>
#include <qtest_kde.h>

#include "modeltest.h"
#include <resources/ResourcesModel.h>
#include <resources/ResourcesProxyModel.h>
#include <Category/Category.h>
#include <MuonBackendsFactory.h>
#include <backends/ApplicationBackend/ApplicationBackend.h>

QTEST_KDEMAIN_CORE( ResourcesModelTest )

ResourcesModelTest::ResourcesModelTest()
{
    MuonBackendsFactory f;
    m_appBackend = qobject_cast<ApplicationBackend*>(f.backend("appsbackend"));
    QVERIFY(m_appBackend); //TODO: test all backends
    QTest::kWaitForSignal(m_appBackend, SIGNAL(backendReady()));
    
    m_model = new ResourcesModel(this);
    m_model->addResourcesBackend(m_appBackend);
    new ModelTest(m_model, m_model);
}

ResourcesModelTest::~ResourcesModelTest()
{
    delete m_backend;
}

void ResourcesModelTest::testReload()
{
    QVector<AbstractResource*> apps = m_appBackend->allResources();
    QCOMPARE(apps.count(), m_model->rowCount());
    
    QVector<QVariant> appNames(apps.size());
    for(int i=0; i<m_model->rowCount(); ++i) {
        AbstractResource* app = apps[i];
        appNames[i]=app->property("packageName");
    }
    
    m_appBackend->reload();
    m_appBackend->updatesCount();
    QCOMPARE(apps, m_appBackend->allResources() );
    
    QVERIFY(!apps.isEmpty());
    QCOMPARE(apps.count(), m_model->rowCount());
    
    for(int i=0; i<m_model->rowCount(); ++i) {
        AbstractResource* app = apps[i];
        QCOMPARE(appNames[i], app->property("packageName"));
//         QCOMPARE(m_model->data(m_model->index(i), ResourcesModel::NameRole).toString(), app->name());
    }
}

void ResourcesModelTest::testCategories()
{
    ResourcesProxyModel* proxy = new ResourcesProxyModel(m_model);
    proxy->setSourceModel(m_model);
    QList<Category*> categories = Category::populateCategories();
    foreach(Category* cat, categories) {
        proxy->setFiltersFromCategory(cat);
        qDebug() << "fuuuuuu" << proxy->rowCount() << cat->name();
    }
}
