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
#include <qjson/serializer.h>
#include <QFile>
#include <QDebug>

#ifdef QAPT_ENABLED
#include <ApplicationBackend/ApplicationBackend.h>
#endif

#ifdef ATTICA_ENABLED
#include <KNSBackend/KNSBackend.h>
#endif

MuonExporter::MuonExporter()
    : QObject(0)
{
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
    ApplicationBackend* applicationBackend = new ApplicationBackend(this);
    applicationBackend->integrateMainWindow(this);
    backends += applicationBackend;
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
    if(m_backendsToInitialize==0)
        exportModel();
}

QVariantMap itemDataToMap(const QMap<int, QVariant>& data, const QHash< int, QByteArray >& names)
{
    QVariantMap ret;
    for(QMap<int, QVariant>::const_iterator it = data.constBegin(), itEnd=data.constEnd(); it!=itEnd; ++it) {
        ret.insert(names.value(it.key()), it.value());
    }
    return ret;
}

void MuonExporter::exportModel()
{
    QVariantList data;
    ResourcesModel* m = ResourcesModel::global();
    QHash< int, QByteArray > names = m->roleNames();
    
    for(int i = 0; i<m->rowCount(); i++) {
        QModelIndex idx = m->index(i, 0);
        data += itemDataToMap(m->itemData(idx), names);
        qDebug() << "fuuu" << i << data.last();
    }
    qDebug() << "found items: " << data.count();
    QFile f(m_path.toLocalFile());
    if(f.open(QIODevice::WriteOnly)) {
        bool ok=true;
        QJson::Serializer s;
        s.serialize(data, &f, &ok);
        if(!ok)
            qWarning() << "Could not completely export the data to " << m_path;
    } else {
        qWarning() << "Could not write to " << m_path;
    }
}
