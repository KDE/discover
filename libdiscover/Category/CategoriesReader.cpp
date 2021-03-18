/*
 *   SPDX-FileCopyrightText: 2013 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "CategoriesReader.h"
#include "Category.h"
#include "libdiscover_debug.h"
#include <QCoreApplication>
#include <QDomNode>
#include <QFile>
#include <QStandardPaths>

#include <DiscoverBackendsFactory.h>
#include <resources/AbstractResourcesBackend.h>

QVector<Category *> CategoriesReader::loadCategoriesFile(AbstractResourcesBackend *backend)
{
    QString path = QStandardPaths::locate(QStandardPaths::GenericDataLocation,
                                          QLatin1String("libdiscover/categories/") + backend->name() + QLatin1String("-categories.xml"));
    if (path.isEmpty()) {
        auto cat = backend->category();
        if (cat.isEmpty())
            qCWarning(LIBDISCOVER_LOG) << "Couldn't find a category for " << backend->name();

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

    QDomDocument menuDocument;
    QString error;
    int line;
    bool correct = menuDocument.setContent(&menuFile, &error, &line);
    if (!correct)
        qCWarning(LIBDISCOVER_LOG) << "error while parsing the categories file:" << error << " at: " << path << ':' << line;

    QDomElement root = menuDocument.documentElement();

    QDomNode node = root.firstChild();
    while (!node.isNull()) {
        if (node.nodeType() == QDomNode::ElementNode) {
            ret << new Category({path}, qApp);
            ret.last()->parseData(path, node);
        }

        node = node.nextSibling();
    }
    Category::sortCategories(ret);
    return ret;
}
