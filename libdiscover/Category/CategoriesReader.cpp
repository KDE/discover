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
#include <QDebug>
#include <qstandardpaths.h>

#include <DiscoverBackendsFactory.h>
#include <resources/AbstractResourcesBackend.h>

QList<Category*> CategoriesReader::loadCategoriesFile(const QString& name)
{
    QList<Category *> ret;
    QString path = QStandardPaths::locate(QStandardPaths::GenericDataLocation, "libmuon/categories/"+name+"-categories.xml");
    if (path.isEmpty()) {
        qWarning() << "Couldn't find a category for " << name;
        return ret;
    }

    QFile menuFile(path);
    if (!menuFile.open(QIODevice::ReadOnly)) {
        // Broken install or broken FS
        return ret;
    }

    QDomDocument menuDocument;
    QString error;
    int line;
    bool correct = menuDocument.setContent(&menuFile, &error, &line);
    if(!correct)
        qWarning() << "error while parsing the categories file:" << error << " at: " << path << ':' << line;

    QDomElement root = menuDocument.documentElement();

    QDomNode node = root.firstChild();
    while(!node.isNull())
    {
        ret << new Category( {name} );
        ret.last()->parseData(path, node, true);

        node = node.nextSibling();
    }
    return ret;
}

static bool categoryLessThan(Category *c1, const Category *c2)
{
    return (QString::localeAwareCompare(c1->name(), c2->name()) < 0);
}

QList<Category*> CategoriesReader::populateCategories()
{
    DiscoverBackendsFactory f;
    QStringList backendNames = f.allBackendNames();

    QList<Category*> ret;
    Q_FOREACH (const QString& name, backendNames) {
        QList<Category*> cats = loadCategoriesFile(name);

        if(ret.isEmpty()) {
            ret += cats;
        } else {
            Q_FOREACH (Category* c, cats)
                Category::addSubcategory(ret, c);
        }
    }
    qSort(ret.begin(), ret.end(), categoryLessThan);
    return ret;
}
