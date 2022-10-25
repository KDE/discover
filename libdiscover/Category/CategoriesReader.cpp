/*
 *   SPDX-FileCopyrightText: 2013 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "CategoriesReader.h"
#include "Category.h"
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
        auto cat = backend->category();
        if (cat.isEmpty())
            qCDebug(LIBDISCOVER_LOG) << "Couldn't find a category for " << backend->name();

        Category::sortCategories(cat);
        return cat;
    }
    return loadCategoriesPath(path);
}

QVector<Category *> CategoriesReader::loadCategoriesPath(const QString &path)
{
    QVector<Category *> ret;
    QFile menuFile(path);
    if (!menuFile.open(QIODevice::ReadOnly)) {
        qCWarning(LIBDISCOVER_LOG) << "couldn't open" << path;
        return ret;
    }

    QXmlStreamReader xml(&menuFile);
    xml.readNextStartElement(); // We want to skip the first <Menu> overall

    while (!xml.atEnd() && !xml.hasError()) {
        xml.readNext();

        if (xml.isStartElement() && xml.name() == QLatin1String("Menu")) {
            ret << new Category({path}, qApp);
            ret.last()->parseData(path, &xml);
        }
    }

    if (xml.hasError()) {
        qCWarning(LIBDISCOVER_LOG) << "error while parsing the categories file:" << path << ':' << xml.lineNumber() << xml.errorString();
    }

    Category::sortCategories(ret);
    return ret;
}
