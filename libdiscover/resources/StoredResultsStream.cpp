/*
 *   SPDX-FileCopyrightText: 2016 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "StoredResultsStream.h"


StoredResultsStream::StoredResultsStream(const QSet< ResultsStream* >& streams)
    : AggregatedResultsStream(streams)
{
    connect(this, &ResultsStream::resourcesFound, this, [this](const QVector<AbstractResource*>& resources) {
        for(auto r : resources)
            connect(r, &QObject::destroyed, this, [this, r](){
                m_resources.removeAll(r);
            });
        m_resources += resources;
    });

    connect(this, &AggregatedResultsStream::finished, this, [this]() { Q_EMIT finishedResources(m_resources); });
}

QVector< AbstractResource* > StoredResultsStream::resources() const
{
    return m_resources;
}

