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

#include "KNSUpdater.h"
#include "KNSBackend.h"
#include <resources/AbstractResource.h>

KNSUpdater::KNSUpdater(KNSBackend* parent)
    : AbstractBackendUpdater(parent)
    , m_backend(parent)
{}

bool KNSUpdater::hasUpdates() const
{
    return m_backend->updatesCount() > 0;
}

void KNSUpdater::start()
{
    QList< AbstractResource* > resources = m_backend->upgradeablePackages();
    foreach(AbstractResource* res, resources) {
        m_backend->installApplication(res);
    }
    emit updatesFinnished();
}

qreal KNSUpdater::progress() const
{
    return hasUpdates() ? 0 : 1;
}

long unsigned int KNSUpdater::remainingTime() const
{
    return 0;
}
