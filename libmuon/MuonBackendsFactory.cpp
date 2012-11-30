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
#include <KServiceTypeTrader>
#include <KDebug>
#include <QPluginLoader>
#include <kplugininfo.h>

MuonBackendsFactory::MuonBackendsFactory()
{}

AbstractResourcesBackend* MuonBackendsFactory::backend(const QString& pluginname)
{
    QString query = QString("[X-KDE-PluginInfo-Name]=='%1'").arg( pluginname );
    KService::List serviceList = KServiceTypeTrader::self()->query( "Muon/Backend", query );
    if(!serviceList.isEmpty()) {
        return backendForPlugin(KPluginInfo(serviceList.first()));
    } else {
        qWarning() << "Couldn't find the backend: " << pluginname;
    }
    return 0;
}

QList<AbstractResourcesBackend*> MuonBackendsFactory::allBackends()
{
    KService::List serviceList = KServiceTypeTrader::self() ->query("Muon/Backend");
    
    QList<AbstractResourcesBackend*> ret;
    foreach(const KService::Ptr& plugin, serviceList) {
        KPluginInfo info(plugin);
        
        AbstractResourcesBackend* b = backendForPlugin(info);
        if(b) {
            ret += b;
        } else {
            qDebug() << "couldn't load " << info.name();
        }
    }

    if(ret.isEmpty())
        qWarning() << "Didn't find any muon backend!";
    return ret;
}


AbstractResourcesBackend* MuonBackendsFactory::backendForPlugin(const KPluginInfo& info)
{
    QVariantMap args;
    foreach(const QString& prop, info.service()->propertyNames()) {
        args[prop] = info.property(prop);
    }
    
    QString str_error;
    AbstractResourcesBackend* obj = KServiceTypeTrader::createInstanceFromQuery<AbstractResourcesBackend>(
        QLatin1String( "Muon/Backend" ), QString::fromLatin1( "[X-KDE-PluginInfo-Name]=='%1'" ).arg( info.pluginName() ),
        ResourcesModel::global(), QVariantList() << args, &str_error );
    
    if(!obj) {
        qDebug() << "error when loading the plugin" << info.name() << "because" << str_error;
    }
    return obj;
}

