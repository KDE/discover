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

#include "ApplicationBackendTest.h"
#include <QStringList>
#include <QAction>
#include <QFile>
#include <KProtocolManager>
#include <KActionCollection>
#include <qtest.h>

#include <tests/modeltest.h>
#include <ApplicationBackend.h>
#include <resources/ResourcesModel.h>
#include <resources/ResourcesProxyModel.h>
#include <resources/AbstractBackendUpdater.h>
#include <Category/Category.h>
#include <Category/CategoryModel.h>
#include <DiscoverBackendsFactory.h>
#include <KXmlGuiWindow>
#include <QAptActions.h>

QTEST_MAIN( ApplicationBackendTest )

QString getCodename(const QString& value)
{
    QString ret;
    QFile f(QStringLiteral("/etc/os-release"));
    if(f.open(QIODevice::ReadOnly|QIODevice::Text)){
	QRegExp rx(QStringLiteral("%1=(.+)\n").arg(value));
	while(!f.atEnd()) {
	    QString line = QString::fromUtf8(f.readLine());
	    if(rx.exactMatch(line)) {
		ret = rx.cap(1);
		break;
	    }
	}
    }
    return ret;
}

AbstractResourcesBackend* backendByName(ResourcesModel* m, const QString& name)
{
    QVector<AbstractResourcesBackend*> backends = m->backends();
    foreach(AbstractResourcesBackend* backend, backends) {
        if(QString::fromLatin1(backend->metaObject()->className())==name) {
            return backend;
        }
    }
    return nullptr;
}

ApplicationBackendTest::ApplicationBackendTest()
{
    QString ratingsDir = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation)+QStringLiteral("/libdiscover/ratings.txt");
    QFile testRatings(QStringLiteral("~/.kde-unit-test/share/apps/libdiscover/ratings.txt"));
    QFile ratings(ratingsDir);
    QString codeName = getCodename(QStringLiteral("ID"));
    if(!testRatings.exists()) {
        if(ratings.exists()) {
            ratings.copy(testRatings.fileName());
        } else {
            ratings.close();
            if(codeName.toLower() == QLatin1String("ubuntu")) {
                ratingsDir = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation)+QStringLiteral("/libdiscover/rnrtestratings.txt");
            } else {
                ratingsDir = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation)+QStringLiteral("/libdiscover/popcontestratings.txt");
            }
            ratings.setFileName(ratingsDir);
            if(ratings.exists()) {
                ratings.copy(testRatings.fileName());
            }
        }
        testRatings.close();
        ratings.close();
    }
    ResourcesModel* m = new ResourcesModel(QStringLiteral("qapt-backend"), this);
    m_window = new KActionCollection(this, QStringLiteral("ApplicationBackendTest"));
    m->integrateActions(m_window);
    new ModelTest(m,m);
    m_appBackend = backendByName(m, QStringLiteral("ApplicationBackend"));
    QVERIFY(m_appBackend); //TODO: test all backends
    QSignalSpy s(m, SIGNAL(allInitialized()));
    QVERIFY(s.wait());
}

ApplicationBackendTest::~ApplicationBackendTest()
{
    delete m_window;
}

void ApplicationBackendTest::testReload()
{
    ResourcesModel* model = ResourcesModel::global();
    QVector<AbstractResource*> apps = m_appBackend->allResources();
    QCOMPARE(apps.count(), model->rowCount());
    
    QVector<QVariant> appNames(apps.size());
    for(int i=0; i<model->rowCount(); ++i) {
        AbstractResource* app = apps[i];
        appNames[i]=app->property("packageName");
    }
    
    bool b = QMetaObject::invokeMethod(m_appBackend, "reload");
    QVERIFY(b);
    m_appBackend->updatesCount();
    QCOMPARE(apps, m_appBackend->allResources() );
    
    QVERIFY(!apps.isEmpty());
    QCOMPARE(apps.count(), model->rowCount());
    
    for(int i=0; i<model->rowCount(); ++i) {
        AbstractResource* app = apps[i];
        QCOMPARE(appNames[i], app->property("packageName"));
//         QCOMPARE(m_model->data(m_model->index(i), ResourcesModel::NameRole).toString(), app->name());
    }
}

void ApplicationBackendTest::testCategories()
{
    ResourcesProxyModel* proxy = new ResourcesProxyModel(this);
    CategoryModel* categoryModel = new CategoryModel(proxy);
    categoryModel->setDisplayedCategory(nullptr);
    for(int i=0; i<categoryModel->rowCount(); ++i) {
        Category* cat = categoryModel->categoryForRow(i);
        proxy->setFiltersFromCategory(cat);
    }
}

void ApplicationBackendTest::testRefreshUpdates()
{
    ResourcesModel* m = ResourcesModel::global();

    QSignalSpy spy(m, SIGNAL(fetchingChanged()));
    QAptActions::self()->actionCollection()->action(QStringLiteral("update"))->trigger();
//     QTest::kWaitForSignal(m, SLOT(fetchingChanged()));
    QVERIFY(!m->isFetching());
    qDebug() << spy.count();
}
