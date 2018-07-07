/***************************************************************************
 *   Copyright © 2013 Aleix Pol Gonzalez <aleixpol@blue-systems.com>       *
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
       if(!FwupdCheck())
           qWarning() << "Error In Install!";
    }
    else if(role == RemoveRole)
    {
        if(!FwupdRemove())
           qWarning() << "Error in Remove!";
    }

    iterateTransaction();
}

FwupdTransaction::~FwupdTransaction()
{
    
}

bool FwupdTransaction::FwupdCheck()
{
    g_autoptr(GCancellable) cancellable = g_cancellable_new();
    g_autoptr(GError) error = NULL;
    
    if(m_app->isDeviceLocked)
    {
        const gchar *device_id;
        device_id = m_app->m_deviceID.toUtf8().constData();
        if(device_id == NULL)
        {
            qWarning("No Device ID Set");
            return false;
        }
        if (!fwupd_client_unlock (m_backend->client, device_id,cancellable, &error))
        {
            m_backend->FwupdHandleError(&error);
            return false;
        }
        return true;
     }
    if(!FwupdInstall())
    {
        // To DO error handling
        return false;
    }
    return true;
    
}

bool FwupdTransaction::FwupdInstall()
{
    const gchar *device_id;
    FwupdInstallFlags install_flags = FWUPD_INSTALL_FLAG_NONE;//Removed 0 check for ussage
    GFile *local_file;
    g_autofree gchar *filename = NULL;
    g_autoptr(GCancellable) cancellable = g_cancellable_new();
    g_autoptr(GError) error = NULL;
    
    local_file = m_app->m_file;
    
    if(local_file == NULL)
    {
        //to Do error handling
        qWarning("No Local File Set For this Resource");
        return false;
    }
    
    filename = g_file_get_path (local_file);
    
    if (!g_file_query_exists (local_file, cancellable)) 
    {
        const gchar *uri = m_app->m_updateURI.toUtf8().constData();
         if(!m_backend->FwupdDownloadFile(uri,filename))
            return false;
    }
    
    /* limit to single device? */
    device_id = m_app->m_deviceID.toUtf8().constData();
    if (device_id == NULL)
        device_id = FWUPD_DEVICE_ID_ANY;

    /* only offline supported */
    if (m_app->isOnlyOffline)
        install_flags = FWUPD_INSTALL_FLAG_OFFLINE; // removed the bit wise or operation |=

    if (!fwupd_client_install (m_backend->client, device_id,filename, install_flags,cancellable, &error)) {
        m_backend->FwupdHandleError(&error);
        return false;
    }
    return true;
}

bool FwupdTransaction::FwupdRemove()
{
    // To Do Implement It
    return true;
}

int FwupdTransaction::speed()
{
    //To Do Implement It
    return 0;
}

void FwupdTransaction::iterateTransaction()
{
    if (!m_iterate)
        return;

    setStatus(CommittingStatus);
    if(progress()<100) {
        setProgress(fwupd_client_get_percentage (m_backend->client));
        
        QTimer::singleShot(100, this, &FwupdTransaction::iterateTransaction);
    } else
#ifdef TEST_PROCEED
        Q_EMIT proceedRequest(QStringLiteral("yadda yadda"), QStringLiteral("Biii BOooo<ul><li>A</li><li>A</li><li>A</li><li>A</li></ul>"));
#else
        finishTransaction();
#endif
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
