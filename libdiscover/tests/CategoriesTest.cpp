/*
 *   SPDX-FileCopyrightText: 2015 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include <Category/CategoriesReader.h>
#include <Category/Category.h>
#include <QList>
#include <QTest>

#include <KLocalizedString>

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
            QFINDTESTDATA("../backends/PackageKitBackend/packagekit-backend-categories.xml"),
            QFINDTESTDATA("../backends/FlatpakBackend/flatpak-backend-categories.xml"),
            QFINDTESTDATA("../backends/DummyBackend/dummy-backend-categories.xml"),
        };

        QVector<Category *> ret;
        CategoriesReader reader;
        for (const QString &name : categoryFiles) {
            qDebug() << "doing..." << name;
            const QVector<Category *> cats = reader.loadCategoriesPath(name, Category::Localization::Force);

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

    void testTranslations_data()
    {
        QTest::addColumn<QString>("language");

        auto languages = KLocalizedString::availableDomainTranslations("libdiscover").values();
        QVERIFY(languages.size() > 1); // at least one more than en_US!
        std::ranges::sort(languages);
        for (const auto &language : languages) {
            QTest::newRow(qUtf8Printable(language)) << language;
        }
    }

    void testTranslations()
    {
        // Make sure loading translations doesn't explode. Specifically because we have requirements for unique category names, so this
        // would for example fail when a translation has ambiguous category translations. When that happens we need to inform the
        // relevant l10n team.

        QFETCH(QString, language);

        KLocalizedString::setLanguages({language});
        populateCategories();
    }
};

QTEST_MAIN(CategoriesTest)

#include "CategoriesTest.moc"
