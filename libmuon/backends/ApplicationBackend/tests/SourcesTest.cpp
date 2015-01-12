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

#include "SourcesTest.h"
#include "modeltest.h"
#include <ReviewsBackend/ReviewsModel.h>
#include <qapt/backend.h>
#include <KProtocolManager>
#include <qtest.h>
#include <ReviewsBackend/AbstractReviewsBackend.h>
#include <MuonBackendsFactory.h>
#include <resources/AbstractResourcesBackend.h>
#include <resources/ResourcesModel.h>
#include <resources/SourcesModel.h>
#include <resources/AbstractSourcesBackend.h>
#include <MuonMainWindow.h>
#include <QSignalSpy>

QTEST_MAIN( SourcesTest )

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

SourcesTest::SourcesTest(QObject* parent): QObject(parent)
{
    ResourcesModel* m = new ResourcesModel("muon-applications-backend", this);
    m_window = new MuonMainWindow;
    m->integrateMainWindow(m_window);
    m_appBackend = backendByName(m, "ApplicationBackend");
    QVERIFY(QSignalSpy(m, SIGNAL(allInitialized())).wait());
}

void SourcesTest::testSourcesFetch()
{
    SourcesModel* sources = SourcesModel::global();
    QVERIFY(sources->rowCount() == 1);
    QObject* l = sources->data(sources->index(0), SourcesModel::SourceBackend).value<QObject*>();
    AbstractSourcesBackend* backend = qobject_cast<AbstractSourcesBackend*>(l);
    QVERIFY(!backend->name().isEmpty());
    QAbstractItemModel* aptSources = backend->sources();
    
    for(int i = 0, c=aptSources->rowCount(); i<c; ++i) {
        QVERIFY(aptSources);
        QModelIndex idx = aptSources->index(i, 0);
        QVERIFY(idx.data(Qt::DisplayRole).toString() != QString());
    }
}
