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

#include "OriginsBackendTest.h"
#include <QtTest/QtTest>

QTEST_MAIN( OriginsBackendTest )

OriginsBackendTest::OriginsBackendTest(QObject* parent)
    : QObject(parent)
{
}

void OriginsBackendTest::testLoad()
{
    {
    QFile f("testsource.list");
    QVERIFY(f.open(QFile::WriteOnly|QFile::Text));
    f.write("deb file:/home/jason/debian stable main contrib non-free\n");
    f.write("deb ftp://ftp.debian.org/debian stable contrib\n");
    f.write("deb http://nonus.debian.org/debian-non-US stable/non-US main contrib non-free\n");
    f.write("deb http://ftp.de.debian.org/debian-non-US unstable/binary-$(ARCH)/\n");
    f.write("deb [arch=i386,amd64] http://ftp.de.debian.org/debian-non-US unstable\n");
    f.write("#hola companys!\n");
    f.write("       #com anem?\n");
    f.write("       \n");
    f.write("\n");
    f.close();
    }
    
    OriginsBackend origins;
    origins.load("testsource.list");
    
    QCOMPARE(origins.sources().size(), 5);
    foreach(Source* s, origins.sources()) {
        QVERIFY(!s->uri().isEmpty());
        QVERIFY(!s->uri().contains(']'));
    }
    
    QFile::remove("testsource.list");
}

void OriginsBackendTest::testLocal()
{
    OriginsBackend origins;
    origins.load("/etc/apt/sources.list");
    QDir d("/etc/apt/sources.list.d/");
    foreach(const QString& f, d.entryList(QStringList("*.list"))) {
        origins.load(d.filePath(f));
    }
}
