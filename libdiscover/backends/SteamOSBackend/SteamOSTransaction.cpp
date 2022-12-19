/*
 *   SPDX-FileCopyrightText: 2022 Jeremy Whiting <jeremy.whiting@collabora.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "SteamOSTransaction.h"
#include "SteamOSResource.h"
#include <KLocalizedString>
#include <QDebug>
#include <QTimer>
#include <QtGlobal>

#include "SteamOSBackend.h"
#include "dbusproperties_interface.h"

SteamOSTransaction::SteamOSTransaction(SteamOSResource *app, Transaction::Role role, ComSteampoweredAtomupd1Interface *interface)
    : Transaction(app->backend(), app, role, {})
    , m_app(app)
    , m_interface(interface)
{
    setCancellable(true);
    setStatus(Status::SetupStatus);

    auto steamosProperties = new OrgFreedesktopDBusPropertiesInterface(SteamOSBackend::service(), SteamOSBackend::path(), QDBusConnection::systemBus(), this);
    connect(steamosProperties,
            &OrgFreedesktopDBusPropertiesInterface::PropertiesChanged,
            this,
            [this](const QString &interface_name, const QVariantMap &changed_properties, const QStringList &invalidated_properties) {
                if (interface_name != SteamOSBackend::service()) {
                    return;
                }

                auto changed = [&](const QString &property) {
                    return changed_properties.contains(property) || invalidated_properties.contains(property);
                };
                if (changed("ProgressPercentage")) {
                    // Get percentage and pass on to gui
                    double percent = m_interface->progressPercentage();
                    qDebug() << "steamos-backend: Progress percentage: " << percent;
                    setProgress(qBound(0.0, percent, 100.0));
                }
                if (changed("EstimatedCompletionTime")) {
                    qulonglong timeRemaining = m_interface->estimatedCompletionTime();
                    qDebug() << "steamos-backend: Estimated completion time: " << timeRemaining;
                    setRemainingTime(timeRemaining);
                }
                if (changed("UpdateStatus")) {
                    refreshStatus();
                }
            });

    m_interface->StartUpdate(m_app->getBuild());
    refreshStatus();
}

void SteamOSTransaction::cancel()
{
    if (!m_interface) {
        // This should never happen
        qWarning() << "steamos-backend: Error: No DBus interface provided to cancel. Please file a bug.";
        return;
    }

    m_interface->CancelUpdate();

    setStatus(CancelledStatus);
}

void SteamOSTransaction::finishTransaction()
{
    AbstractResource::State newState;
    switch (role()) {
    case InstallRole:
    case ChangeAddonsRole:
        newState = AbstractResource::Installed;
        Q_EMIT needReboot();
        break;
    case RemoveRole:
        newState = AbstractResource::None;
        break;
    }
    m_app->setState(newState);
    setStatus(DoneStatus);
    deleteLater();
}

void SteamOSTransaction::refreshStatus()
{
    // Get update state and update our state
    uint status = m_interface->updateStatus();
    qDebug() << "steamos-backend: New state: " << status;

    // Status is one of these from the xml definition:
    //    0 = IDLE, the update has not been launched yet
    //    1 = IN_PROGRESS, the update is currently being applied
    //    2 = PAUSED, the update has been paused
    //    3 = SUCCESSFUL, the update process successfully completed
    //    4 = FAILED, an error occurred during the update
    //    5 = CANCELLED, a special case of FAILED where the update attempt has been cancelled
    switch (status) {
    case 0: // IDLE
        break;
    case 1: // IN_PROGRESS
        setStatus(Status::DownloadingStatus);
        break;
    case 2: // PAUSED
        setStatus(Status::QueuedStatus);
        break;
    case 3: // SUCCESSFUL
        setStatus(Status::DoneStatus);
        finishTransaction();
        break;
    case 4: // FAILED
        setStatus(Status::DoneWithErrorStatus);
        finishTransaction();
        break;
    case 5: // CANCELLED
        setStatus(Status::CancelledStatus);
        finishTransaction();
        break;
    }
}
