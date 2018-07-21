/***************************************************************************
 *   Copyright © 2013 Aleix Pol Gonzalez <aleixpol@blue-systems.com>       *
 *   Copyright © 2018 Abhijeet Sharma <sharma.abhijeet2096@gmail.com>      *
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

#include "FwupdTransaction.h"

#include <QTimer>
#include <QDebug>
#include <KRandom>


#define TEST_PROCEED

FwupdTransaction::FwupdTransaction(FwupdResource* app, FwupdBackend* backend, Role role)
    : FwupdTransaction(app, backend,{}, role)
{
}

FwupdTransaction::FwupdTransaction(FwupdResource* app, FwupdBackend* backend, const AddonList& addons, Transaction::Role role)
    : Transaction(app->backend(), app, role, addons)
    , m_app(app)
    , m_backend(backend)
{
    setCancellable(true);
    if(role == InstallRole)
    {
        setStatus(QueuedStatus);
        if(!check())
            qWarning() << "Fwupd Error: Error In Install!";
    }
    else if(role == RemoveRole)
    {
        if(!remove())
           qWarning() << "Fwupd Error: Error in Remove!";
    }

    iterateTransaction();
}

FwupdTransaction::~FwupdTransaction()
{
    
}

bool FwupdTransaction::check()
{
    g_autoptr(GCancellable) cancellable = g_cancellable_new();
    g_autoptr(GError) error = NULL;
    
    if(m_app->isDeviceLocked)
    {
        QString device_id = m_app->m_deviceID;
        if(device_id.isNull())
        {
            qWarning("Fwupd Error: No Device ID set, cannot unlock device ");
            return false;
        }
        if (!fwupd_client_unlock (m_backend->client, device_id.toUtf8().constData(),cancellable, &error))
        {
            m_backend->handleError(&error);
            return false;
        }
        return true;
     }
    if(!install())
    {
        qWarning("Fwupd Error: Cannot Install Application");
        return false;
    }
    return true;
    
}

bool FwupdTransaction::install()
{
    FwupdInstallFlags install_flags = FWUPD_INSTALL_FLAG_NONE;
    g_autoptr(GCancellable) cancellable = g_cancellable_new();
    g_autoptr(GError) error = NULL;
    
    QFile *localFile = m_app->m_file;
    QString deviceId = m_app->m_deviceID;
    
    if(!localFile)
    {
        qWarning("Fwupd Error: No Local File Set For this Resource");
        return false;
    }
     
    if (!(localFile->exists())) 
    {
        const QUrl uri(m_app->m_updateURI);
        setStatus(DownloadingStatus);
        if(!m_backend->downloadFile(uri,localFile->fileName()))
            return false;
    }
    
    /* limit to single device? */
    if (deviceId.isNull())
        deviceId = QStringLiteral(FWUPD_DEVICE_ID_ANY);

    /* only offline supported */
    if (m_app->isOnlyOffline)
        install_flags = static_cast<FwupdInstallFlags>(install_flags | FWUPD_INSTALL_FLAG_OFFLINE);
   
    m_iterate = true;
    QTimer::singleShot(100, this, &FwupdTransaction::iterateTransaction);
    if (!fwupd_client_install (m_backend->client, deviceId.toUtf8().constData(),localFile->fileName().toUtf8().constData(), install_flags,cancellable, &error)) 
    {
        m_backend->handleError(&error);
        m_iterate = false;
        return false;
    }
    finishTransaction();
    return true;
}

bool FwupdTransaction::remove()
{
    m_app->setState(AbstractResource::State::None);
    return true;
}

void FwupdTransaction::iterateTransaction()
{
    if (!m_iterate)
        return;

    setStatus(CommittingStatus);
    if(progress()<100) 
    {
        setProgress(fwupd_client_get_percentage (m_backend->client));
        QTimer::singleShot(100, this, &FwupdTransaction::iterateTransaction);
    }
}

void FwupdTransaction::proceed()
{
    finishTransaction();
}

void FwupdTransaction::cancel()
{
    m_iterate = false;

    setStatus(CancelledStatus);
}

void FwupdTransaction::finishTransaction()
{
    AbstractResource::State newState;
    switch(role()) {
    case InstallRole:
    case ChangeAddonsRole:
        newState = AbstractResource::Installed;
        break;
    case RemoveRole:
        newState = AbstractResource::None;
        break;
    }
    m_app->setAddons(addons());
    m_app->setState(newState);
    setStatus(DoneStatus);
    deleteLater();
}
