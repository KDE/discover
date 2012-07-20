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
#include <ApplicationBackend.h>
#include <resources/ResourcesModel.h>

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
    , m_backend(0)
    , m_applicationBackend(0)
    , m_mainWindow(0)
{}

ResourcesModel* BackendsSingleton::appsModel()
{
    if(!m_appsModel) {
        m_appsModel = new ResourcesModel(this);
    }
    return m_appsModel;
}

void BackendsSingleton::initialize(QApt::Backend* b, MuonDiscoverMainWindow* main)
{
    m_backend = b;
    m_mainWindow = main;
    emit initialized();
}

QApt::Backend* BackendsSingleton::backend()
{
    return m_backend;
}

QMainWindow* BackendsSingleton::mainWindow() const
{
    return m_mainWindow;
}

ApplicationBackend* BackendsSingleton::applicationBackend()
{
    foreach(AbstractResourcesBackend* b, m_appsModel->backends()) {
        ApplicationBackend* appbackend = qobject_cast<ApplicationBackend*>(b);
        if(qobject_cast<ApplicationBackend*>(b))
            return appbackend;
    }
    return 0;
}
