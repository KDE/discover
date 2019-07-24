/***************************************************************************
 *   Copyright © 2013 Aleix Pol Gonzalez <aleixpol@blue-systems.com>       *
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

#include "CategoriesReader.h"
#include "Category.h"
#include <QDomNode>
#include <QFile>
#include "libdiscover_debug.h"
#include <QStandardPaths>
#include <QCoreApplication>

#include <DiscoverBackendsFactory.h>
#include <resources/AbstractResourcesBackend.h>

QVector<Category*> CategoriesReader::loadCategoriesFile(AbstractResourcesBackend* backend)
{
    QString path = QStandardPaths::locate(QStandardPaths::GenericDataLocation, QStringLiteral("libdiscover/categories/")+backend->name()+QStringLiteral("-categories.xml"));
    if (path.isEmpty()) {
        auto cat = backend->category();
        if (cat.isEmpty())
            qCWarning(LIBDISCOVER_LOG) << "Couldn't find a category for " << backend->name();

        Category::sortCategories(cat);
        return cat;
    }
    return loadCategoriesPath(path);
}

QVector<Category*> CategoriesReader::loadCategoriesPath(const QString& path)
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
    if(!correct)
        qCWarning(LIBDISCOVER_LOG) << "error while parsing the categories file:" << error << " at: " << path << ':' << line;

    QDomElement root = menuDocument.documentElement();

    QDomNode node = root.firstChild();
    while(!node.isNull())
    {
        if (node.nodeType() == QDomNode::ElementNode) {
            ret << new Category( {path}, qApp );
            ret.last()->parseData(path, node);
        }

        node = node.nextSibling();
    }
    Category::sortCategories(ret);
    return ret;
}
