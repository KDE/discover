/***************************************************************************
 *   Copyright © 2014 Aleix Pol Gonzalez <aleixpol@blue-systems.com>       *
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

#include "AbstractSourcesBackend.h"
#include "AbstractResourcesBackend.h"
#include <QAbstractItemModel>

AbstractSourcesBackend::AbstractSourcesBackend(AbstractResourcesBackend* parent)
    : QObject(parent)
{}

AbstractSourcesBackend::~AbstractSourcesBackend() = default;

AbstractResourcesBackend * AbstractSourcesBackend::resourcesBackend() const
{
    return dynamic_cast<AbstractResourcesBackend*>(parent());
}


bool AbstractSourcesBackend::moveSource(const QString& sourceId, int delta)
{
    Q_UNUSED(sourceId)
    Q_UNUSED(delta)
    return false;
}

QString AbstractSourcesBackend::firstSourceId() const
{
    auto m = const_cast<AbstractSourcesBackend*>(this)->sources();
    return m->index(0, 0).data(AbstractSourcesBackend::IdRole).toString();
}

QString AbstractSourcesBackend::lastSourceId() const
{
    auto m = const_cast<AbstractSourcesBackend*>(this)->sources();
    return m->index(m->rowCount()-1, 0).data(AbstractSourcesBackend::IdRole).toString();
}
