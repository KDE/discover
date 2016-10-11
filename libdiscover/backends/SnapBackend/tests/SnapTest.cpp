/***************************************************************************
 *   Copyright © 2016 Aleix Pol Gonzalez <aleixpol@blue-systems.com>       *
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

#include <resources/ResourcesUpdatesModel.h>
#include <UpdateModel/UpdateModel.h>
#include <tests/modeltest.h>
#include <resources/ResourcesModel.h>
#include <resources/ResourcesProxyModel.h>
#include <resources/AbstractBackendUpdater.h>
#include <ApplicationAddonsModel.h>
#include <Transaction/TransactionModel.h>
#include <ReviewsBackend/ReviewsModel.h>
#include <ScreenshotsModel.h>
#include "../SnapSocket.h"

#include <QtTest>
#include <QAction>

class SnapTest : public QObject
{
Q_OBJECT
public:
    SnapTest() : socket(this) {}

private Q_SLOTS:
    void things() {
        QSignalSpy spy(&socket, &SnapSocket::connectedChanged);
        QVERIFY(socket.isConnected() || spy.wait());
        QVERIFY(socket.isConnected());

        //Need to have installed snaps for this test to do something
        const auto snaps = socket.snaps();
        for(const auto &snapValue : snaps) {
            QVERIFY(snapValue.isObject());
            const auto snap = snapValue.toObject();
            QVERIFY(snap.contains(QLatin1String("name") ));
            QVERIFY(snap.contains(QLatin1String("developer")));

            const auto requestedSnap = socket.snapByName(snap.value(QLatin1String("name")).toString().toUtf8());
            QCOMPARE(requestedSnap, snap);
        }
    }

private:
    SnapSocket socket;
};


QTEST_MAIN(SnapTest)

#include "SnapTest.moc"
