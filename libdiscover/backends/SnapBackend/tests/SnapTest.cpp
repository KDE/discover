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

namespace QTest {
    template<>
    char *toString(const QJsonValue &value)
    {
        QByteArray ret;
        if (value.isObject())
            ret = QJsonDocument(value.toObject()).toJson();
        else if (value.isArray())
            ret = QJsonDocument(value.toArray()).toJson();
        else
            ret = value.toString().toLatin1();
        return qstrdup(ret.data());
    }

    template<>
    char *toString(const QJsonObject &object)
    {
        QByteArray ret = QJsonDocument(object).toJson();
        return qstrdup(ret.data());
    }
}

class SnapTest : public QObject
{
Q_OBJECT
public:
    SnapTest() : socket(this) {}

private Q_SLOTS:
    void initTestCase()
    {
        QSignalSpy spy(&socket, &SnapSocket::connectedChanged);
        QVERIFY(socket.isConnected() || spy.wait());
        QVERIFY(socket.isConnected());
    }

    void listing()
    {
        //Need to have installed snaps for this test to do something
        const auto snaps = socket.snaps();
        for(const auto &snapValue : snaps) {
            QVERIFY(snapValue.isObject());
            auto snap = snapValue.toObject();
            QVERIFY(snap.contains(QLatin1String("name") ));
            QVERIFY(snap.contains(QLatin1String("developer")));

            auto requestedSnap = socket.snapByName(snap.value(QLatin1String("name")).toString().toUtf8());

            //should treat these separately becauase they're randomly delivered in different order
            //just make sure they're the same number, for now
            const auto apps = snap.take(QLatin1String("apps")).toArray(), reqApps = requestedSnap.take(QLatin1String("apps")).toArray();
            QCOMPARE(apps.count(), reqApps.count());

            if (requestedSnap != snap) {
                const auto keys = snap.keys();
                QCOMPARE(requestedSnap.keys(), keys);
                foreach(const auto &key, keys) {
                    QCOMPARE(requestedSnap.value(key), snap.value(key));
                }
            }
            QCOMPARE(requestedSnap, snap);
        }
    }

    void find()
    {
        const auto snaps = socket.find(QStringLiteral("editor"));
        QVERIFY(snaps.count() > 1);
        for(const auto &snapValue : snaps) {
            QVERIFY(snapValue.isObject());
            const auto snap = snapValue.toObject();
            QVERIFY(snap.contains(QLatin1String("name") ));
            QVERIFY(snap.contains(QLatin1String("developer")));
        }
    }

private:
    SnapSocket socket;
};


QTEST_MAIN(SnapTest)

#include "SnapTest.moc"
