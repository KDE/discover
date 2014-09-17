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
#include <QPluginLoader>
#include <QCoreApplication>
#include <QDebug>
#include <QSet>
#include <kplugininfo.h>

Q_GLOBAL_STATIC(QSet<QString>, s_requestedBackends)
Q_DECLARE_METATYPE(KService::Ptr);

void MuonBackendsFactory::setRequestedBackends(const QStringList& backends)
{
    *s_requestedBackends = backends.toSet();
}

MuonBackendsFactory::MuonBackendsFactory()
{}

AbstractResourcesBackend* MuonBackendsFactory::backend(const QString& name) const
{
    QString query = QString("[X-KDE-PluginInfo-Name]=='%1'").arg( name );
    KService::List serviceList = KServiceTypeTrader::self()->query( "Muon/Backend", query );
    if(!serviceList.isEmpty()) {
        return backendForPlugin(KPluginInfo(serviceList.first()));
    } else {
        KService::List serviceList = KServiceTypeTrader::self()->query("Muon/Backend");
        QStringList backends;
        foreach(const KService::Ptr& ptr, serviceList) {
            backends += KPluginInfo(ptr).pluginName();
        }
        qWarning() << "Couldn't find the backend: " << name << "among" << backends;
    }
    return 0;
}

QStringList MuonBackendsFactory::allBackendNames() const
{
    QSet<QString> whitelist = fetchBackendsWhitelist();
    if (!whitelist.isEmpty())
        return whitelist.toList();

    QStringList ret;
    KService::List serviceList = KServiceTypeTrader::self()->query("Muon/Backend");
    foreach(const KService::Ptr& service, serviceList) {
        ret += service->property("X-KDE-PluginInfo-Name").toString();
    }
    return ret;
}

QList<AbstractResourcesBackend*> MuonBackendsFactory::allBackends() const
{
    QList<AbstractResourcesBackend*> ret;
    QSet<QString> whitelist = fetchBackendsWhitelist();
    if(!whitelist.isEmpty()) {
        foreach(const QString& b, whitelist)
            ret += backend(b);
    } else {
        KService::List serviceList = KServiceTypeTrader::self()->query("Muon/Backend");
        
        foreach(const KService::Ptr& plugin, serviceList) {
            KPluginInfo info(plugin);
            if(info.pluginName()=="muon-dummy-backend")
                continue;

            AbstractResourcesBackend* b = backendForPlugin(info);
            if(b) {
                ret += b;
            } else {
                qDebug() << "couldn't load " << info.name();
            }
        }
    }

    if(ret.isEmpty())
        qWarning() << "Didn't find any muon backend!";
    return ret;
}

int MuonBackendsFactory::backendsCount() const
{
    int ret = fetchBackendsWhitelist().count();
    if(ret>0)
        return ret;
    
    KService::List serviceList = KServiceTypeTrader::self() ->query("Muon/Backend");
    
    foreach(const KService::Ptr& plugin, serviceList) {
        KPluginInfo info(plugin);
        if(info.name()!="muon-dummybackend")
            ret++;
    }
    return ret;
}

AbstractResourcesBackend* MuonBackendsFactory::backendForPlugin(const KPluginInfo& info) const
{
    QString str_error;
    AbstractResourcesBackend* obj = info.service()->createInstance<AbstractResourcesBackend>(ResourcesModel::global(), QVariantList() << qVariantFromValue(info.service()), &str_error);
    
    if(!obj) {
        qDebug() << "error when loading the plugin" << info.name() << "because" << str_error;
    } else if (!obj->isValid()) {
        qDebug() << "Error initializing Muon Backend.";

        // Clean up invalid plugin as we will not load it
        delete obj;
        obj = nullptr;
    }

    return obj;
}

QSet<QString> MuonBackendsFactory::fetchBackendsWhitelist() const
{
    return *s_requestedBackends;
}
