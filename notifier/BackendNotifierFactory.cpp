/*
 *   SPDX-FileCopyrightText: 2013 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "BackendNotifierFactory.h"
#include <BackendNotifierModule.h>
#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QPluginLoader>

BackendNotifierFactory::BackendNotifierFactory() = default;

QList<BackendNotifierModule *> BackendNotifierFactory::allBackends() const
{
    QList<BackendNotifierModule *> ret;

    foreach (const QString &path, QCoreApplication::instance()->libraryPaths()) {
        QDir dir(path + QStringLiteral("/discover-notifier/"));
        foreach (const QString &file, dir.entryList(QDir::Files)) {
            QString fullPath = dir.absoluteFilePath(file);
            QPluginLoader loader(fullPath);
            loader.load();
            ret += qobject_cast<BackendNotifierModule *>(loader.instance());
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
