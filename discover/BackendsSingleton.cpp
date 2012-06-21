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
#include "OCSBackend/OCSBackend.h"
#include "KNSBackend/KNSBackend.h"
#include <LibQApt/Backend>
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
    , m_ocsBackend(0)
{}

ApplicationBackend* BackendsSingleton::applicationBackend()
{
    if(m_backend && !m_applicationBackend) {
        m_applicationBackend = new ApplicationBackend;
        m_applicationBackend->setBackend(m_backend);
//         appsModel()->addResourcesBackend(applicationBackend());
    }
    
    return m_applicationBackend;
}

ResourcesModel* BackendsSingleton::appsModel()
{
    if(!m_appsModel) {
        m_appsModel = new ResourcesModel(this);
//         m_appsModel->addResourcesBackend(ocsBackend());
        m_appsModel->addResourcesBackend(new KNSBackend("comic.knsrc", this));
    }
    return m_appsModel;
}

AbstractResourcesBackend* BackendsSingleton::ocsBackend()
{
    if(!m_ocsBackend) {
        m_ocsBackend = new OCSBackend(this);
    }
    return m_ocsBackend;
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

