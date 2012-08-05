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

#include "BackendsSingleton.h"
#include "MuonDiscoverMainWindow.h"
#include <resources/ResourcesModel.h>

#ifdef QAPT_ENABLED
#include <ApplicationBackend/ApplicationBackend.h>
#include <QAptIntegration.h>
#include <QTimer>
#endif

#ifdef ATTICA_ENABLED
#include <KNSBackend/KNSBackend.h>
#endif

BackendsSingleton* BackendsSingleton::m_self = 0;

BackendsSingleton* BackendsSingleton::self()
{
    if(!m_self) {
        m_self = new BackendsSingleton;
    }
    return m_self;
}

BackendsSingleton::BackendsSingleton()
    : m_appsModel(0)
{}

ResourcesModel* BackendsSingleton::appsModel()
{
    if(!m_appsModel) {
        m_appsModel = new ResourcesModel(this);
    }
    return m_appsModel;
}

void BackendsSingleton::initialize(MuonDiscoverMainWindow* w)
{
    QList<AbstractResourcesBackend*> backends;

#ifdef ATTICA_ENABLED
    backends += new KNSBackend("comic.knsrc", "face-smile-big", this);
    backends += new KNSBackend("plasmoids.knsrc", "plasma", this);
#endif
    
#ifdef QAPT_ENABLED
    ApplicationBackend* applicationBackend = new ApplicationBackend(this);
    applicationBackend->integrateMainWindow(w);
    backends += applicationBackend;
#endif
    
    foreach(AbstractResourcesBackend* b, backends) {
        appsModel()->addResourcesBackend(b);
    }
}
