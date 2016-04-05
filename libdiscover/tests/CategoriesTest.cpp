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
#include <QList>
#include <Category/Category.h>
#include <Category/CategoriesReader.h>

class CategoriesTest : public QObject
{
    Q_OBJECT
public:
    CategoriesTest() {}

    QVector<Category*> populateCategories()
    {
        const QVector<QString> categoryFiles = {
            QFINDTESTDATA("../backends/BodegaBackend/muon-bodegawallpapers-backend-categories.xml"),
            QFINDTESTDATA("../backends/PackageKitBackend/packagekit-backend-categories.xml"),
            QFINDTESTDATA("../backends/AkabeiBackend/akabei-backend-categories.xml"),
            QFINDTESTDATA("../backends/KNSBackend/knscomic-backend-categories.xml"),
            QFINDTESTDATA("../backends/KNSBackend/knsplasmoids-backend-categories.xml"),
            QFINDTESTDATA("../backends/ApplicationBackend/ubuntu_sso_dbus_interface.xml"),
            QFINDTESTDATA("../backends/ApplicationBackend/qapt-backend-categories.xml"),
            QFINDTESTDATA("../backends/DummyBackend/dummy-backend-categories.xml")
        };

        QVector<Category*> ret;
        CategoriesReader reader;
        Q_FOREACH (const QString& name, categoryFiles) {
            const QVector<Category*> cats = reader.loadCategoriesPath(name);

            if(ret.isEmpty()) {
                ret = cats;
            } else {
                Q_FOREACH (Category* c, cats)
                    Category::addSubcategory(ret, c);
            }
        }
        qSort(ret.begin(), ret.end(), CategoriesReader::categoryLessThan);
        return ret;
    }

private Q_SLOTS:
    void testReadCategories() {
        auto categories = populateCategories();
        QVERIFY(!categories.isEmpty());
    }
};

QTEST_MAIN( CategoriesTest )

#include "CategoriesTest.moc"
