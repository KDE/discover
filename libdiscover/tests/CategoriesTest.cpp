/*
 *   SPDX-FileCopyrightText: 2015 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include <Category/CategoriesReader.h>
#include <Category/Category.h>
#include <QList>
#include <QTest>

class CategoriesTest : public QObject
{
    Q_OBJECT
public:
    CategoriesTest()
    {
    }

    QList<Category *> populateCategories()
    {
        const QList<QString> categoryFiles = {
            QFINDTESTDATA("../backends/PackageKitBackend/packagekit-backend-categories.xml"),
            QFINDTESTDATA("../backends/FlatpakBackend/flatpak-backend-categories.xml"),
            QFINDTESTDATA("../backends/DummyBackend/dummy-backend-categories.xml"),
        };

        QList<Category *> ret;
        CategoriesReader reader;
        for (const QString &name : categoryFiles) {
            qDebug() << "doing..." << name;
            const QList<Category *> cats = reader.loadCategoriesPath(name);

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

        for (Category *c : categories) {
            if (c->name() != QLatin1String("Dummy Category"))
                continue;

            auto filter = c->filter();
            QVERIFY(filter.type == CategoryFilter::CategoryNameFilter);
            QVERIFY(std::get<QString>(filter.value) == QLatin1String("dummy"));
        }
    }
};

QTEST_MAIN(CategoriesTest)

#include "CategoriesTest.moc"
