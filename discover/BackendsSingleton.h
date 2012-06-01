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

#ifndef BACKENDSSINGLETON_H
#define BACKENDSSINGLETON_H
#include <LibQApt/Globals>

class ResourcesModel;
class MuonInstallerMainWindow;
namespace QApt { class Backend; }
class ApplicationBackend;
class ApplicationModel;

class BackendsSingleton : public QObject
{
Q_OBJECT
public:
    BackendsSingleton();
    
    static BackendsSingleton* self();
    
    void initialize(QApt::Backend* b, MuonInstallerMainWindow* main);
    
    ResourcesModel* appsModel();
    QApt::Backend* backend();
    ApplicationBackend* applicationBackend();
    MuonInstallerMainWindow* mainWindow() const;

signals:
    void initialized();
    
private:
    static BackendsSingleton* m_self;
    ResourcesModel* m_appsModel;
    QApt::Backend* m_backend;
    QApt::CacheState m_originalState;
    ApplicationBackend* m_applicationBackend;
    MuonInstallerMainWindow* m_mainWindow;
};

#endif // BACKENDSSINGLETON_H
