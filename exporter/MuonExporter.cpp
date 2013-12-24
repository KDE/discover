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
#include <QFile>
#include <QDebug>
#include <QTimer>
#include <QMetaProperty>
#include <qjsondocument.h>

MuonExporter::MuonExporter()
    : QObject(0)
{
    m_startExportingTimer = new QTimer(this);
    m_startExportingTimer->setInterval(200);
    m_startExportingTimer->setSingleShot(true);
    connect(m_startExportingTimer, SIGNAL(timeout()), SLOT(exportModel()));
    
    m_exculdedProperties += "executables";
    m_exculdedProperties += "canExecute";
    connect(ResourcesModel::global(), SIGNAL(allInitialized()), SLOT(allBackendsInitialized()));
}

MuonExporter::~MuonExporter()
{}

void MuonExporter::allBackendsInitialized()
{
    m_startExportingTimer->start();
    connect(ResourcesModel::global(), SIGNAL(rowsInserted(QModelIndex,int,int)), m_startExportingTimer, SLOT(start()));
}

void MuonExporter::setExportPath(const QUrl& url)
{
    m_path = url;
}

QVariantMap itemDataToMap(const AbstractResource* res, const QSet<QString>& excluded)
{
    QVariantMap ret;
    int propsCount = res->metaObject()->propertyCount();
    for(int i = 0; i<propsCount; i++) {
        QMetaProperty prop = res->metaObject()->property(i);
        if(prop.type() == QVariant::UserType || excluded.contains(prop.name()))
            continue;
        QVariant val = res->property(prop.name());
        
        if(val.isNull())
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
        data += itemDataToMap(res, m_exculdedProperties);
    }
    qDebug() << "found items: " << data.count();
    QJsonDocument doc = QJsonDocument::fromVariant(data);
    if(doc.isNull()) {
        qWarning() << "Could not completely export the data to " << m_path;
        return;
    }

    QFile f(m_path.toLocalFile());
    if(f.open(QIODevice::WriteOnly|QIODevice::Text)) {
        bool ok=true;
        int w = f.write(doc.toJson(QJsonDocument::Indented));
        if(w<=0)
            qWarning() << "Could not completely export the data to " << m_path;
    } else {
        qWarning() << "Could not write to " << m_path;
    }
    emit exportDone();
}
