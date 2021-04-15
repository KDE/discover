/*
 *   SPDX-FileCopyrightText: 2013 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *   SPDX-FileCopyrightText: 2018 Abhijeet Sharma <sharma.abhijeet2096@gmail.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "FwupdTransaction.h"

#include <QTimer>
#include <resources/AbstractBackendUpdater.h>

FwupdTransaction::FwupdTransaction(FwupdResource *app, FwupdBackend *backend)
    : Transaction(backend, app, Transaction::InstallRole, {})
    , m_app(app)
    , m_backend(backend)
{
    setCancellable(true);
    setStatus(QueuedStatus);

    Q_ASSERT(!m_app->deviceId().isEmpty());
    QTimer::singleShot(0, this, &FwupdTransaction::install);
}

FwupdTransaction::~FwupdTransaction() = default;

void FwupdTransaction::install()
{
    g_autoptr(GError) error = nullptr;

    if (m_app->isDeviceLocked()) {
        QString device_id = m_app->deviceId();
        if (device_id.isEmpty()) {
            qWarning() << "Fwupd Error: No Device ID set, cannot unlock device " << this << m_app->name();
        } else if (!fwupd_client_unlock(m_backend->client, device_id.toUtf8().constData(), nullptr, &error)) {
            m_backend->handleError(error);
        }
        setStatus(DoneWithErrorStatus);
        return;
    }

    const QString fileName = m_app->cacheFile();
    if (!QFileInfo::exists(fileName)) {
        const QUrl uri(m_app->updateURI());
        setStatus(DownloadingStatus);
        QNetworkAccessManager *manager = new QNetworkAccessManager(this);
        auto reply = manager->get(QNetworkRequest(uri));
        QFile *file = new QFile(fileName);
        if (!file->open(QFile::WriteOnly)) {
            qWarning() << "Fwupd Error: Could not open to write" << fileName << uri;
            setStatus(DoneWithErrorStatus);
            file->deleteLater();
            return;
        }

        connect(reply, &QNetworkReply::finished, this, [this, file, reply]() {
            file->close();
            file->deleteLater();

            if (reply->error() != QNetworkReply::NoError) {
                qWarning() << "Fwupd Error: Could not download" << reply->url() << reply->errorString();
                file->remove();
                setStatus(DoneWithErrorStatus);
            } else {
                fwupdInstall(file->fileName());
            }
        });
        connect(reply, &QNetworkReply::readyRead, this, [file, reply]() {
            file->write(reply->readAll());
        });
    } else {
        fwupdInstall(fileName);
    }
}

void FwupdTransaction::fwupdInstall(const QString &file)
{
    FwupdInstallFlags install_flags = FWUPD_INSTALL_FLAG_NONE;
    g_autoptr(GError) error = nullptr;

    /* only offline supported */
    if (m_app->isOnlyOffline())
        install_flags = static_cast<FwupdInstallFlags>(install_flags | FWUPD_INSTALL_FLAG_OFFLINE);

    if (!fwupd_client_install(m_backend->client, m_app->deviceId().toUtf8().constData(), file.toUtf8().constData(), install_flags, nullptr, &error)) {
        m_backend->handleError(error);
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
    switch (role()) {
    case InstallRole:
    case ChangeAddonsRole:
        newState = AbstractResource::Installed;
        break;
    case RemoveRole:
        newState = AbstractResource::None;
        break;
    }
    m_app->setState(newState);
    if (m_app->needsReboot()) {
        m_app->backend()->backendUpdater()->enableNeedsReboot();
    }
    setStatus(DoneStatus);
    deleteLater();
}
