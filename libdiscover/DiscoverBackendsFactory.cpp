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

#include "DiscoverBackendsFactory.h"
#include "resources/AbstractResourcesBackend.h"
#include "resources/ResourcesModel.h"
#include "utils.h"
#include "libdiscover_debug.h"
#include <QStandardPaths>
#include <QDir>
#include <QCommandLineParser>
#include <QPluginLoader>
#include <QDirIterator>
#include <KSharedConfig>
#include <KConfigGroup>
#include <KDesktopFile>
#include <KLocalizedString>

Q_GLOBAL_STATIC(QStringList, s_requestedBackends)

void DiscoverBackendsFactory::setRequestedBackends(const QStringList& backends)
{
    *s_requestedBackends = backends;
}

bool DiscoverBackendsFactory::hasRequestedBackends()
{
    return !s_requestedBackends->isEmpty();
}

DiscoverBackendsFactory::DiscoverBackendsFactory()
{}

QVector<AbstractResourcesBackend*> DiscoverBackendsFactory::backend(const QString& name) const
{
    if (QDir::isAbsolutePath(name) && QStandardPaths::isTestModeEnabled()) {
        return backendForFile(name, QFileInfo(name).fileName());
    } else {
        return backendForFile(name, name);
    }
}

QVector<AbstractResourcesBackend*> DiscoverBackendsFactory::backendForFile(const QString& libname, const QString& name) const
{
    QPluginLoader* loader = new QPluginLoader(QStringLiteral("discover/") + libname, ResourcesModel::global());

    //     qCDebug(LIBDISCOVER_LOG) << "trying to load plugin:" << loader->fileName();
    AbstractResourcesBackendFactory* f = qobject_cast<AbstractResourcesBackendFactory*>(loader->instance());
    if(!f) {
        qCWarning(LIBDISCOVER_LOG) << "error loading" << libname << loader->errorString() << loader->metaData();
        return {};
    }
    auto instances = f->newInstance(ResourcesModel::global(), name);
    if(instances.isEmpty()) {
        qCWarning(LIBDISCOVER_LOG) << "Couldn't find the backend: " << libname << "among" << allBackendNames(false, true);
        return instances;
    }

    return instances;
}

QStringList DiscoverBackendsFactory::allBackendNames(bool whitelist, bool allowDummy) const
{
    if (whitelist) {
        QStringList whitelistNames = *s_requestedBackends;
        if (!whitelistNames.isEmpty())
            return whitelistNames;
    }

    QStringList pluginNames;
    foreach (const QString &dir, QCoreApplication::libraryPaths()) {
        QDirIterator it(dir + QStringLiteral("/discover"), QDir::Files);
        while (it.hasNext()) {
            it.next();
            if (QLibrary::isLibrary(it.fileName()) && (allowDummy || it.fileName() != QLatin1String("dummy-backend.so"))) {
                pluginNames += it.fileInfo().baseName();
            }
        }
    }

    pluginNames.removeDuplicates(); //will happen when discover is installed twice on the system
    return pluginNames;
}

QVector<AbstractResourcesBackend*> DiscoverBackendsFactory::allBackends() const
{
    QStringList names = allBackendNames();
    auto ret = kTransform<QVector<AbstractResourcesBackend*>>(names, [this](const QString& name) { return backend(name); });
    ret.removeAll(nullptr);

    if(ret.isEmpty())
        qCWarning(LIBDISCOVER_LOG) << "Didn't find any Discover backend!";
    return ret;
}

int DiscoverBackendsFactory::backendsCount() const
{
    return allBackendNames().count();
}

void DiscoverBackendsFactory::setupCommandLine(QCommandLineParser* parser)
{
    parser->addOption(QCommandLineOption(QStringLiteral("backends"), i18n("List all the backends we'll want to have loaded, separated by comma ','."), QStringLiteral("names")));
}

void DiscoverBackendsFactory::processCommandLine(QCommandLineParser* parser, bool test)
{
    *s_requestedBackends = test ? QStringList{ QStringLiteral("dummy-backend") } : parser->value(QStringLiteral("backends")).split(QLatin1Char(','), QString::SkipEmptyParts);
}
