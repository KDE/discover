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

#include "BackendNotifierFactory.h"
#include <BackendNotifierModule.h>
#include <QDebug>
#include <QCoreApplication>
#include <QDir>
#include <QPluginLoader>

BackendNotifierFactory::BackendNotifierFactory()
{}

QList<BackendNotifierModule*> BackendNotifierFactory::allBackends() const
{
    QList<BackendNotifierModule*> ret;

    for(const QString& path : QCoreApplication::instance()->libraryPaths()) {
        QDir dir(path + QStringLiteral("/discover-notifier/"));
        for(const QString& file : dir.entryList(QDir::Files)) {
            QString fullPath = dir.absoluteFilePath(file);
            QPluginLoader loader(fullPath);
            loader.load();
            ret += qobject_cast<BackendNotifierModule*>(loader.instance());
            if (ret.last() == nullptr) {
                qWarning() << "couldn't load" << fullPath << "because" << loader.errorString();
                ret.removeLast();
            }
        }
    }
    if (ret.isEmpty())
        qWarning() << "couldn't find any notifier backend" << QCoreApplication::instance()->libraryPaths();

    return ret;
}
