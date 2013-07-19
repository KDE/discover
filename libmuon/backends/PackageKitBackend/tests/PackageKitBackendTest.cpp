/*
    <one line to give the program's name and a brief idea of what it does.>
    Copyright (C) 2013  <copyright holder> <email>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "PackageKitBackendTest.h"
#include "PackageKitBackend.h"

#include <qtest_kde.h>

QTEST_KDEMAIN_CORE( PackageKitBackendTest )

PackageKitBackendTest::PackageKitBackendTest(QObject * parent)
  : QObject(parent)
{

}

PackageKitBackendTest::~PackageKitBackendTest()
{

}

void PackageKitBackendTest::testVersionComparator_data()
{
    QTest::addColumn<QString>("version1");
    QTest::addColumn<QString>("version2");
    QTest::addColumn<int>("expected");

    QTest::newRow("easy test") << "1" << "2" << -1;
    QTest::newRow("normal version") << "1.2.5" << "1.2.3" << 1;
    QTest::newRow("normal version inverted") << "1.2.1" << "2.4.4" << -1;
    QTest::newRow("normal easy =") << "1.2.3" << "1.2.3" << 0;
     
    QTest::newRow("two versions") << "4.5-2.3.1" << "4.5-2.1.3" << 1;
    QTest::newRow("two digits") << "3.4-2.13.2" << "3.4-2.3.2" << 1;
    QTest::newRow("two digits inverted") << "2.3-2.9.1" << "2.3-2.13.1" << -1;
    
    QTest::newRow("MozillaBranding-opensuse") << "21-2.5.1" << "6.1-2.2.1" << 1;
    QTest::newRow("MozillaBranding-opensuse") << "21-2.5.1" << "20" << 1;
}

void PackageKitBackendTest::testVersionComparator()
{
    QFETCH(QString, version1);
    QFETCH(QString, version2);
    QFETCH(int, expected);
    
    QCOMPARE(PackageKitBackend::compare_versions(version1, version2), expected);
}
