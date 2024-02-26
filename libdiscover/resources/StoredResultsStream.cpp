/*
 *   SPDX-FileCopyrightText: 2016 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "StoredResultsStream.h"

StoredResultsStream::StoredResultsStream(const QSet<ResultsStream *> &streams)
    : AggregatedResultsStream(streams)
{
    connect(this, &ResultsStream::resourcesFound, this, [this](const QVector<StreamResult> &resources) {
        for (auto r : resources)
            connect(r.resource, &QObject::destroyed, this, [this, r]() {
                for (auto it = m_results.begin(); it != m_results.end(); ++it) {
                    if (r.resource == it->resource)
                        it = m_results.erase(it);
                    else
                        ++it;
                }
            });
        m_results += resources;
    });

    connect(this, &AggregatedResultsStream::finished, this, [this]() {
        Q_EMIT finishedResources(m_results);
    });
}

QVector<StreamResult> StoredResultsStream::resources() const
{
    return m_results;
}

#include "moc_StoredResultsStream.cpp"
