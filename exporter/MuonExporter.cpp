/*
 *   SPDX-FileCopyrightText: 2012 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "MuonExporter.h"
#include <resources/AbstractResourcesBackend.h>
#include <resources/ResourcesModel.h>
#include <resources/StoredResultsStream.h>
#include <resources/AbstractResource.h>
#include <QFile>
#include <QDebug>
#include <QTimer>
#include <QMetaProperty>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>

MuonExporter::MuonExporter()
    : QObject(nullptr)
    , m_exculdedProperties({ "executables" , "canExecute" })
{
    connect(ResourcesModel::global(), &ResourcesModel::backendsChanged, this, &MuonExporter::fetchResources);
}

MuonExporter::~MuonExporter() = default;

void MuonExporter::setExportPath(const QUrl& url)
{
    m_path = url;
}

QJsonObject itemDataToMap(const AbstractResource* res, const QSet<QByteArray>& excluded)
{
    QJsonObject ret;
    int propsCount = res->metaObject()->propertyCount();
    for(int i = 0; i<propsCount; i++) {
        QMetaProperty prop = res->metaObject()->property(i);
        if(prop.type() == QVariant::UserType || excluded.contains(prop.name()))
            continue;

        const QVariant val = prop.read(res);
        if(val.isNull())
            continue;
        
        ret.insert(QLatin1String(prop.name()), QJsonValue::fromVariant(val));
    }
    return ret;
}

void MuonExporter::fetchResources()
{
    ResourcesModel* m = ResourcesModel::global();
    QSet<ResultsStream*> streams;
    foreach(auto backend, m->backends()) {
        streams << backend->search({});
    }
    auto stream = new StoredResultsStream(streams);
    connect(stream, &StoredResultsStream::finishedResources, this, &MuonExporter::exportResources);
    QTimer::singleShot(15000, stream, &AggregatedResultsStream::finished);
}

void MuonExporter::exportResources(const QVector<AbstractResource*>& resources)
{
    QJsonArray data;
    foreach(auto res, resources) {
        data += itemDataToMap(res, m_exculdedProperties);
    }

    QJsonDocument doc = QJsonDocument(data);
    if(doc.isNull()) {
        qWarning() << "Could not completely export the data to " << m_path;
        return;
    }

    QFile f(m_path.toLocalFile());
    if(f.open(QIODevice::WriteOnly|QIODevice::Text)) {
        int w = f.write(doc.toJson(QJsonDocument::Indented));
        if(w<=0)
            qWarning() << "Could not completely export the data to " << m_path;
    } else {
        qWarning() << "Could not write to " << m_path;
    }
    qDebug() << "exported items: " << data.count() << " to " << m_path;
    emit exportDone();
}
