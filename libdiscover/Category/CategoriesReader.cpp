/*
 *   SPDX-FileCopyrightText: 2013 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "CategoriesReader.h"
#include "libdiscover_debug.h"
#include <QCoreApplication>
#include <QFile>
#include <QStandardPaths>
#include <QXmlStreamReader>

#include <DiscoverBackendsFactory.h>
#include <resources/AbstractResourcesBackend.h>

QVector<Category *> CategoriesReader::loadCategoriesFile(AbstractResourcesBackend *backend)
{
    QString path = QStandardPaths::locate(QStandardPaths::GenericDataLocation,
                                          QStringLiteral("libdiscover/categories/") + backend->name() + QStringLiteral("-categories.xml"));
    if (path.isEmpty()) {
        auto categories = backend->category();
        if (categories.isEmpty()) {
            qCDebug(LIBDISCOVER_LOG) << "CategoriesReader: Couldn't find a category for" << backend->name();
        }

        Category::sortCategories(categories);
        return categories;
    }
    return loadCategoriesPath(path, Category::Localization::Yes);
}

QVector<Category *> CategoriesReader::loadCategoriesPath(const QString &path, Category::Localization localization)
{
    QVector<Category *> ret;
    qCDebug(LIBDISCOVER_LOG) << "CategoriesReader: Load categories from file" << path << "with l10n" << (localization == Category::Localization::Yes);
    QFile menuFile(path);
    if (!menuFile.open(QIODevice::ReadOnly)) {
        qCWarning(LIBDISCOVER_LOG).nospace().noquote() << "CategoriesReader: Couldn't open the categories file " << path << ": " << menuFile.errorString();
        return ret;
    }

    QXmlStreamReader xml(&menuFile);
    xml.readNextStartElement(); // We want to skip the first <Menu> overall

    while (!xml.atEnd() && !xml.hasError()) {
        xml.readNext();

        if (xml.isStartElement() && xml.name() == QLatin1String("Menu")) {
            ret << new Category({path}, qApp);
            ret.last()->parseData(path, &xml, localization);
        }
    }

    if (xml.hasError()) {
        qCWarning(LIBDISCOVER_LOG).nospace().noquote()
            << "CategoriesReader: Error while parsing the categories file " << path << ':' << xml.lineNumber() << ": " << xml.errorString();
    }

    if (const auto optionalString = Category::duplicatedNamesAsStringNested(ret); optionalString) {
        switch (localization) {
        case Category::Localization::Force:
        case Category::Localization::No:
            Q_ASSERT_X(false, Q_FUNC_INFO, qUtf8Printable(optionalString.value()));
            break;
        case Category::Localization::Yes:
            qCWarning(LIBDISCOVER_LOG) << "Category has duplicates. Reloading without translations!";
            qDeleteAll(ret);
            ret = loadCategoriesPath(path, Category::Localization::No);
            break;
        }
    }
    Category::sortCategories(ret);

    return ret;
}
