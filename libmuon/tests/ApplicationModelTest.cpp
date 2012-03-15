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

QTEST_MAIN( ApplicationModelTest )

ApplicationModelTest::ApplicationModelTest()
{
    m_backend = new QApt::Backend;
}

ApplicationModelTest::~ApplicationModelTest()
{
    delete m_backend;
}

void ApplicationModelTest::testReload()
{
    m_backend->init();

    if (m_backend->xapianIndexNeedsUpdate()) {
        m_backend->updateXapianIndex();
    }

    if (KProtocolManager::proxyType() == KProtocolManager::ManualProxy) {
        m_backend->setWorkerProxy(KProtocolManager::proxyFor("http"));
    }
    ApplicationBackend* appBackend = new ApplicationBackend(this);
    appBackend->setBackend(m_backend);
    
    ApplicationModel* model = new ApplicationModel(this);
    ApplicationProxyModel* updatesProxy = new ApplicationProxyModel(this);
    updatesProxy->setSourceModel(model);
    new ModelTest(model, model);
    new ModelTest(updatesProxy, updatesProxy);
    model->setBackend(appBackend);
    updatesProxy->setStateFilter(QApt::Package::ToUpgrade);
    
    QCOMPARE(updatesProxy->rowCount(), appBackend->updatesCount());
    m_backend->reloadCache();
    QCOMPARE(appBackend->applicationList().count(), model->rowCount());
}
