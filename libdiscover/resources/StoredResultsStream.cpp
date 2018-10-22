/***************************************************************************
 *   Copyright © 2016 Aleix Pol Gonzalez <aleixpol@blue-systems.com>       *
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

