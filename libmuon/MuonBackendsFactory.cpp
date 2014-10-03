/***************************************************************************
 *   Copyright © 2012 Aleix Pol Gonzalez <aleixpol@blue-systems.com>       *
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

#include "MuonBackendsFactory.h"
#include "resources/AbstractResourcesBackend.h"
#include "resources/ResourcesModel.h"
#include <QPluginLoader>
#include <QDebug>
#include <QStandardPaths>
#include <QDir>
#include <QCommandLineParser>
#include <KSharedConfig>
#include <KConfigGroup>
#include <KDesktopFile>
#include <KLocalizedString>

Q_GLOBAL_STATIC(QStringList, s_requestedBackends)

MuonBackendsFactory::MuonBackendsFactory()
{}

AbstractResourcesBackend* MuonBackendsFactory::backend(const QString& name) const
{
    QString data = QStandardPaths::locate(QStandardPaths::GenericDataLocation, QStringLiteral("libmuon/backends/%1.desktop").arg(name));
    KDesktopFile cfg(data);
    KConfigGroup group = cfg.group("Desktop Entry");
    QString libname = group.readEntry("X-KDE-Library", QString());
    QPluginLoader* loader = new QPluginLoader("muon/"+libname, ResourcesModel::global());

//     qDebug() << "trying to load plugin:" << loader->fileName();
    AbstractResourcesBackendFactory* f = qobject_cast<AbstractResourcesBackendFactory*>(loader->instance());
    if(!f) {
        qWarning() << "error loading" << name << loader->errorString() << loader->metaData();
        return 0;
    }
    AbstractResourcesBackend* instance = f->newInstance(ResourcesModel::global());
    if(instance) {
        instance->setMetaData(data);
        return instance;
    } else {
        qWarning() << "Couldn't find the backend: " << name << "among" << allBackendNames(false) << "because" << loader->errorString();
    }
    return 0;
}

QStringList MuonBackendsFactory::allBackendNames(bool whitelist) const
{
    if (whitelist) {
        QStringList whitelist = *s_requestedBackends;
        if (!whitelist.isEmpty())
            return whitelist;
    }

    QStringList ret;
    QStringList dirs = QStandardPaths::locateAll(QStandardPaths::GenericDataLocation, QStringLiteral("libmuon/backends/"), QStandardPaths::LocateDirectory);
    foreach (const QString& dir, dirs) {
        QDir d(dir);
        foreach(const QFileInfo& file, d.entryInfoList(QDir::Files)) {
            if (file.baseName()!="muon-dummy-backend") {
                ret.append(file.baseName());
            }
        }
    }

    return ret;
}

QList<AbstractResourcesBackend*> MuonBackendsFactory::allBackends() const
{
    QList<AbstractResourcesBackend*> ret;
    QStringList names = allBackendNames();
    foreach(const QString& name, names)
        ret += backend(name);

    ret.removeAll(nullptr);

    if(ret.isEmpty())
        qWarning() << "Didn't find any muon backend!";
    return ret;
}

int MuonBackendsFactory::backendsCount() const
{
    return allBackendNames().count();
}

void MuonBackendsFactory::setupCommandLine(QCommandLineParser* parser)
{
    parser->addOption(QCommandLineOption("listbackends", i18n("List all the available backends.")));
    parser->addOption(QCommandLineOption("backends", i18n("List all the backends we'll want to have loaded, separated by coma ','."), "names"));
}

void MuonBackendsFactory::processCommandLine(QCommandLineParser* parser)
{
    *s_requestedBackends = parser->value("backends").split(",", QString::SkipEmptyParts);
    if(parser->isSet("listbackends")) {
        fprintf(stdout, "%s", qPrintable(i18n("Available backends:\n")));
        MuonBackendsFactory f;
        foreach(const QString& name, f.allBackendNames(false))
            fprintf(stdout, " * %s\n", qPrintable(name));
        qApp->exit(0);
    }
}
