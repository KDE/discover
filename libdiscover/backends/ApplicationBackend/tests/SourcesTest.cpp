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
#include <tests/modeltest.h>
#include <ReviewsBackend/ReviewsModel.h>
#include <qapt/backend.h>
#include <KProtocolManager>
#include <KActionCollection>
#include <qtest.h>
#include <ReviewsBackend/AbstractReviewsBackend.h>
#include <DiscoverBackendsFactory.h>
#include <resources/AbstractResourcesBackend.h>
#include <resources/ResourcesModel.h>
#include <resources/SourcesModel.h>
#include <resources/AbstractSourcesBackend.h>
#include <QSignalSpy>

QTEST_MAIN( SourcesTest )

AbstractResourcesBackend* backendByName(ResourcesModel* m, const QString& name)
{
    QVector<AbstractResourcesBackend*> backends = m->backends();
    foreach(AbstractResourcesBackend* backend, backends) {
        if(QString::fromLatin1(backend->metaObject()->className()) == name) {
            return backend;
        }
    }
    return nullptr;
}

SourcesTest::SourcesTest(QObject* parent): QObject(parent)
{
    ResourcesModel* m = new ResourcesModel(QStringLiteral("qapt-backend"), this);
    m_window = new KActionCollection(this, QStringLiteral("SourcesTest"));
    m->integrateActions(m_window);
    m_appBackend = backendByName(m, QStringLiteral("ApplicationBackend"));
    QVERIFY(QSignalSpy(m, SIGNAL(allInitialized())).wait());
    
    SourcesModel* sources = SourcesModel::global();
    QVERIFY(sources->rowCount() == 1);
    QVERIFY(!backend()->name().isEmpty());
}

AbstractSourcesBackend* SourcesTest::backend() const
{
    SourcesModel* sources = SourcesModel::global();
    QObject* l = sources->data(sources->index(0), SourcesModel::SourceBackend).value<QObject*>();
    return qobject_cast<AbstractSourcesBackend*>(l);
}


void SourcesTest::testSourcesFetch()
{
    QAbstractItemModel* aptSources = backend()->sources();
    
    for(int i = 0, c=aptSources->rowCount(); i<c; ++i) {
        QVERIFY(aptSources);
        QModelIndex idx = aptSources->index(i, 0);
        QVERIFY(idx.data(Qt::DisplayRole).toString() != QString());
    }
}

void SourcesTest::testResourcesMatchSources()
{
    QAbstractItemModel* aptSources = backend()->sources();
    QSet<QString> allSources;
    for (int i=0, c=aptSources->rowCount(); i<c; ++i) {
        QModelIndex idx = aptSources->index(i, 0);
        allSources += idx.data(Qt::DisplayRole).toString();
    }
    
    ResourcesModel* rmodel = ResourcesModel::global();
    for (int i=0, c=rmodel->rowCount(); i<c; ++i) {
        QModelIndex idx = rmodel->index(i, 0);
        QString origin = idx.data(ResourcesModel::OriginRole).toString();
        bool found = allSources.contains(origin);
        if (!found) {
            qDebug() << "couldn't find" << origin << "for" << idx.data(ResourcesModel::NameRole).toString() << "@" << i << "/" << c << "in" << allSources;
            QEXPECT_FAIL("", "I need to ask the Kubuntu guys, I don't understand", Continue);
        }
        QVERIFY(found);
    }
}
