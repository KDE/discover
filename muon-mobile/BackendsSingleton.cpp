/*
 *   Copyright (C) 2012 Aleix Pol Gonzalez <aleixpol@kde.org>
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
#include <LibQApt/Backend>
#include <ApplicationModel/ApplicationModel.h>
#include <ApplicationBackend.h>

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

ApplicationBackend* BackendsSingleton::applicationBackend()
{
    if(!m_applicationBackend) {
        m_applicationBackend = new ApplicationBackend;
        m_applicationBackend->setBackend(m_backend);
    }
    
    return m_applicationBackend;
}

ApplicationModel* BackendsSingleton::appsModel()
{
    if(!m_appsModel) {
        m_appsModel = new ApplicationModel; 
        m_appsModel->setBackend(applicationBackend());
    }
    return m_appsModel;
}

void BackendsSingleton::initialize(QApt::Backend* b, MuonInstallerMainWindow* main)
{
    m_backend = b;
    m_mainWindow = main;
}

QApt::Backend* BackendsSingleton::backend()
{
    return m_backend;
}

MuonInstallerMainWindow* BackendsSingleton::mainWindow() const
{
    return m_mainWindow;
}

