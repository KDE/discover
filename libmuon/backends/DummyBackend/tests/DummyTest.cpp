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

#include "DummyTest.h"
#include <modeltest.h>
#include <resources/ResourcesModel.h>
#include <qtest_kde.h>

#include <QtTest>

QTEST_MAIN(DummyTest);

AbstractResourcesBackend* backendByName(ResourcesModel* m, const QString& name)
{
    QVector<AbstractResourcesBackend*> backends = m->backends();
    foreach(AbstractResourcesBackend* backend, backends) {
        if(backend->metaObject()->className()==name) {
            return backend;
        }
    }
    return nullptr;
}

DummyTest::DummyTest(QObject* parent): QObject(parent)
{
    m_model = new ResourcesModel("muon-dummybackend", this);
//     new ModelTest(m_model, m_model);

    m_appBackend = backendByName(m_model, "DummyBackend");
    QVERIFY(m_appBackend); //TODO: test all backends
    QTest::kWaitForSignal(m_appBackend, SIGNAL(backendReady()));
}

void DummyTest::testReadData()
{
    QBENCHMARK {
        for(int i=0; i<m_model->rowCount(); i++) {
            QModelIndex idx = m_model->index(i, 0);
            QVERIFY(!m_model->data(idx, ResourcesModel::NameRole).isNull());
        }
    }
}
