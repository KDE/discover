/*
 *   Copyright (C) 2012 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library/Lesser General Public License
 *   version 2, or (at your option) any later version, as published by the
 *   Free Software Foundation
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details
 *
 *   You should have received a copy of the GNU Library/Lesser General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "MuonExporter.h"
#include <resources/AbstractResourcesBackend.h>
#include <resources/ResourcesModel.h>
#include <resources/AbstractResource.h>
#include <qjson/serializer.h>
#include <QFile>
#include <QDebug>
#include <QTimer>

#ifdef QAPT_ENABLED
#include <ApplicationBackend/ApplicationBackend.h>
#endif

#ifdef ATTICA_ENABLED
#include <KNSBackend/KNSBackend.h>
#endif

MuonExporter::MuonExporter()
    : QObject(0)
{
    m_startExportingTimer = new QTimer(this);
    m_startExportingTimer->setInterval(200);
    m_startExportingTimer->setSingleShot(true);
    connect(m_startExportingTimer, SIGNAL(timeout()), SLOT(exportModel()));
    initialize();
}

MuonExporter::~MuonExporter()
{}

void MuonExporter::initialize()
{
    QList<AbstractResourcesBackend*> backends;

#ifdef ATTICA_ENABLED
    backends += new KNSBackend("comic.knsrc", "face-smile-big", this);
    backends += new KNSBackend("plasmoids.knsrc", "plasma", this);
#endif
    
#ifdef QAPT_ENABLED
    backends += new ApplicationBackend(this);
#endif
    
    m_backendsToInitialize = backends.count();
    ResourcesModel* m = ResourcesModel::global();
    foreach(AbstractResourcesBackend* b, backends) {
        connect(b, SIGNAL(backendReady()), SLOT(backendReady()));
        m->addResourcesBackend(b);
    }
}

void MuonExporter::setExportPath(const KUrl& url)
{
    m_path = url;
}

void MuonExporter::backendReady()
{
    m_backendsToInitialize--;
    if(m_backendsToInitialize==0) {
        m_startExportingTimer->start();
        connect(ResourcesModel::global(), SIGNAL(rowsInserted(QModelIndex,int,int)), m_startExportingTimer, SLOT(start()));
    }
}

QVariantMap itemDataToMap(const AbstractResource* res)
{
    QVariantMap ret;
    int propsCount = res->metaObject()->propertyCount();
    for(int i = 0; i<propsCount; i++) {
        QMetaProperty prop = res->metaObject()->property(i);
        QVariant val = res->property(prop.name());
        if(val.isNull() || val.canConvert<QObject*>())
            continue;
        ret.insert(prop.name(), val);
    }
    return ret;
}

void MuonExporter::exportModel()
{
    QVariantList data;
    ResourcesModel* m = ResourcesModel::global();
    
    for(int i = 0; i<m->rowCount(); i++) {
        QModelIndex idx = m->index(i, 0);
        AbstractResource* res = qobject_cast<AbstractResource*>(m->data(idx, ResourcesModel::ApplicationRole).value<QObject*>());
        Q_ASSERT(res);
        data += itemDataToMap(res);
    }
    qDebug() << "found items: " << data.count();
    QFile f(m_path.toLocalFile());
    if(f.open(QIODevice::WriteOnly|QIODevice::Text)) {
        bool ok=true;
        QJson::Serializer s;
        s.serialize(data, &f, &ok);
        if(!ok)
            qWarning() << "Could not completely export the data to " << m_path;
    } else {
        qWarning() << "Could not write to " << m_path;
    }
    emit exportDone();
}
