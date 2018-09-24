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

FwupdTransaction::FwupdTransaction(FwupdResource* app, FwupdBackend* backend)
    : Transaction(backend, app, Transaction::InstallRole, {})
    , m_app(app)
    , m_backend(backend)
{
    setCancellable(true);
    setStatus(QueuedStatus);

    QTimer::singleShot(0, this, &FwupdTransaction::install);
}

FwupdTransaction::~FwupdTransaction() = default;

void FwupdTransaction::install()
{
    g_autoptr(GError) error = nullptr;

    if(m_app->isDeviceLocked)
    {
        QString device_id = m_app->m_deviceID;
        if(device_id.isNull()) {
            qWarning() << "Fwupd Error: No Device ID set, cannot unlock device " << this;
        } else if(!fwupd_client_unlock(m_backend->client, device_id.toUtf8().constData(),nullptr, &error)) {
            m_backend->handleError(&error);
        }
        setStatus(DoneWithErrorStatus);
        return;
    }

    Q_ASSERT(!m_app->m_file.isEmpty());
    if (m_app->m_file.isEmpty()) {
        setStatus(DoneWithErrorStatus);
        return;
    }

    if(!QFileInfo::exists(m_app->m_file))
    {
        const QUrl uri(m_app->m_updateURI);
        setStatus(DownloadingStatus);
        QNetworkAccessManager *manager = new QNetworkAccessManager(this);
        auto reply = manager->get(QNetworkRequest(uri));
        QFile* file = new QFile(m_app->m_file);

        connect(reply, &QNetworkReply::finished, this, [this, file, reply](){
            file->close();
            file->deleteLater();

            if(reply->error() != QNetworkReply::NoError) {
                qWarning() << "Fwupd Error: Could not download" << reply->url() << reply->errorString();
                file->remove();
                setStatus(DoneWithErrorStatus);
            } else {
                fwupdInstall();
            }
        });
        connect(reply, &QNetworkReply::readyRead, this, [file, reply](){
            file->write(reply->readAll());
        });
    }
    else
    {
        fwupdInstall();
    }
}

void FwupdTransaction::fwupdInstall()
{
    FwupdInstallFlags install_flags = FWUPD_INSTALL_FLAG_NONE;
    g_autoptr(GError) error = nullptr;

    QString localFile = m_app->m_file;
    QString deviceId = m_app->m_deviceID;
    /* limit to single device? */
    if(deviceId.isNull())
        deviceId = QStringLiteral(FWUPD_DEVICE_ID_ANY);

    /* only offline supported */
    if(m_app->isOnlyOffline)
        install_flags = static_cast<FwupdInstallFlags>(install_flags | FWUPD_INSTALL_FLAG_OFFLINE);

    if(!fwupd_client_install(m_backend->client, deviceId.toUtf8().constData(), localFile.toUtf8().constData(), install_flags, nullptr, &error))
    {
        m_backend->handleError(&error);
        setStatus(DoneWithErrorStatus);
    } else
        finishTransaction();
}

void FwupdTransaction::updateProgress()
{
    setProgress(fwupd_client_get_percentage(m_backend->client));
}

void FwupdTransaction::proceed()
{
    finishTransaction();
}

void FwupdTransaction::cancel()
{
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
