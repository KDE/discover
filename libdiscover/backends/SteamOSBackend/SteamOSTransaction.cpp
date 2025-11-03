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
#include "libdiscover_steamos_debug.h"

SteamOSTransaction::SteamOSTransaction(SteamOSResource *app, Transaction::Role role, ComSteampoweredAtomupd1Interface *interface)
    : Transaction(app, role, {})
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
                if (changed(QLatin1String("ProgressPercentage"))) {
                    // Get percentage and pass on to gui
                    double percent = m_interface->progressPercentage();
                    qCDebug(LIBDISCOVER_BACKEND_STEAMOS_LOG) << "steamos-backend: Progress percentage: " << percent;
                    setProgress(qBound(0.0, percent, 100.0));
                }
                if (changed(QLatin1String("EstimatedCompletionTime"))) {
                    qulonglong estimatedCompletion = m_interface->estimatedCompletionTime();
                    QDateTime potentialEndTime = QDateTime::fromSecsSinceEpoch(estimatedCompletion);
                    qCDebug(LIBDISCOVER_BACKEND_STEAMOS_LOG) << "steamos-backend: Estimated completion time:" << potentialEndTime.toString();
                    qulonglong secondsLeft = QDateTime::currentDateTimeUtc().secsTo(potentialEndTime);
                    qCDebug(LIBDISCOVER_BACKEND_STEAMOS_LOG) << "Remaining seconds:" << secondsLeft;
                    setRemainingTime(secondsLeft);
                }
                if (changed(QLatin1String("UpdateStatus"))) {
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

void SteamOSTransaction::finishTransaction(bool installed)
{
    AbstractResource::State newState;
    if (installed) {
        newState = AbstractResource::Installed;
        Q_EMIT needReboot();
        m_app->setState(newState);
        setStatus(DoneStatus);
    }

    // If not installed, don't change Status since it was already set.
    deleteLater();
}

void SteamOSTransaction::refreshStatus()
{
    // Get update state and update our state
    uint status = m_interface->updateStatus();
    qCDebug(LIBDISCOVER_BACKEND_STEAMOS_LOG) << "steamos-backend: New state: " << status;

    switch (status) {
    case SteamOSBackend::Idle:
        break;
    case SteamOSBackend::InProgress:
        setStatus(Status::DownloadingStatus);
        break;
    case SteamOSBackend::Paused:
        setStatus(Status::QueuedStatus);
        break;
    case SteamOSBackend::Successful: // SUCCESSFUL
        setStatus(Status::DoneStatus);
        finishTransaction(true);
        break;
    case SteamOSBackend::Failed: // FAILED
        setStatus(Status::DoneWithErrorStatus);
        finishTransaction(false);
        break;
    case SteamOSBackend::Cancelled: // CANCELLED
        setStatus(Status::CancelledStatus);
        finishTransaction(false);
        break;
    }
}

#include "moc_SteamOSTransaction.cpp"
