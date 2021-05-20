/*
 *   SPDX-FileCopyrightText: 2015 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include <Category/CategoriesReader.h>
#include <Category/Category.h>
#include <QList>
#include <QtTest>

class CategoriesTest : public QObject
{
    Q_OBJECT
public:
    CategoriesTest()
    {
    }

    QVector<Category *> populateCategories()
    {
        const QVector<QString> categoryFiles = {
            QFINDTESTDATA("../backends/BodegaBackend/muon-bodegawallpapers-backend-categories.xml"),
            QFINDTESTDATA("../backends/PackageKitBackend/packagekit-backend-categories.xml"),
            QFINDTESTDATA("../backends/AkabeiBackend/akabei-backend-categories.xml"),
            QFINDTESTDATA("../backends/KNSBackend/knscomic-backend-categories.xml"),
            QFINDTESTDATA("../backends/KNSBackend/knsplasmoids-backend-categories.xml"),
            QFINDTESTDATA("../backends/ApplicationBackend/ubuntu_sso_dbus_interface.xml"),
            QFINDTESTDATA("../backends/ApplicationBackend/qapt-backend-categories.xml"),
            QFINDTESTDATA("../backends/DummyBackend/dummy-backend-categories.xml"),
        };

        QVector<Category *> ret;
        CategoriesReader reader;
        for (const QString &name : categoryFiles) {
            const QVector<Category *> cats = reader.loadCategoriesPath(name);

            if (ret.isEmpty()) {
                ret = cats;
            } else {
                for (Category *c : cats)
                    Category::addSubcategory(ret, c);
            }
        }
        std::sort(ret.begin(), ret.end(), Category::categoryLessThan);
        return ret;
    }

private Q_SLOTS:
    void testReadCategories()
    {
        auto categories = populateCategories();
        QVERIFY(!categories.isEmpty());
    }
};

QTEST_MAIN(CategoriesTest)

#include "CategoriesTest.moc"
