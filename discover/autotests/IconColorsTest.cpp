/*
 *   Copyright (C) 2015 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library/Lesser General Public License
 *   version 2, or (at your option) any later version, as published by the
 *   Free Software Foundation
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details
 *
 *   You should have received a copy of the GNU Library/Lesser General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <QtTest>
#include "../IconColors.h"

class IconColorsTest : public QObject
{
    Q_OBJECT
public:
    IconColorsTest() {}

private slots:
    void testIcon_data() {
        QTest::addColumn<QString>("iconName");
        QTest::addColumn<int>("hue");

        QTest::newRow("akregator") << "akregator" << 15;
        QTest::newRow("korganizer") << "korganizer" << 105;
    }

    void testIcon() {
        QFETCH(QString, iconName);
        QFETCH(int, hue);

        IconColors colors;
        colors.setIconName(iconName);

        QCOMPARE(colors.dominantColor().hue(), hue);
    }
};

QTEST_MAIN( IconColorsTest )

#include "IconColorsTest.moc"
