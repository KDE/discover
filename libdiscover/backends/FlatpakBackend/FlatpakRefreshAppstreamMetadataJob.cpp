/*
 *   SPDX-FileCopyrightText: 2013 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *   SPDX-FileCopyrightText: 2017 Jan Grulich <jgrulich@redhat.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "FlatpakRefreshAppstreamMetadataJob.h"
#include <QDebug>

FlatpakRefreshAppstreamMetadataJob::FlatpakRefreshAppstreamMetadataJob(FlatpakInstallation *installation, FlatpakRemote *remote)
    : QThread()
    , m_cancellable(g_cancellable_new())
    , m_installation(installation)
    , m_remote(remote)
{
    g_object_ref(m_remote);
    connect(this, &FlatpakRefreshAppstreamMetadataJob::finished, this, &QObject::deleteLater);
}

FlatpakRefreshAppstreamMetadataJob::~FlatpakRefreshAppstreamMetadataJob()
{
    g_object_unref(m_remote);
    g_object_unref(m_cancellable);
}

void FlatpakRefreshAppstreamMetadataJob::cancel()
{
    g_cancellable_cancel(m_cancellable);
}

void FlatpakRefreshAppstreamMetadataJob::run()
{
    g_autoptr(GError) localError = nullptr;

#if FLATPAK_CHECK_VERSION(0, 9, 4)
    // With Flatpak 0.9.4 we can use flatpak_installation_update_appstream_full_sync() providing progress reporting which we don't use at this moment, but
    // still better to use newer function in case the previous one gets deprecated
    if (!flatpak_installation_update_appstream_full_sync(m_installation,
                                                         flatpak_remote_get_name(m_remote),
                                                         nullptr,
                                                         nullptr,
                                                         nullptr,
                                                         nullptr,
                                                         m_cancellable,
                                                         &localError)) {
#else
    if (!flatpak_installation_update_appstream_sync(m_installation, flatpak_remote_get_name(m_remote), nullptr, nullptr, m_cancellable, &localError)) {
#endif
        const QString error = localError ? QString::fromUtf8(localError->message) : QStringLiteral("<no error>");
        qWarning() << "Failed to refresh appstream metadata for " << flatpak_remote_get_name(m_remote) << ": " << error;
    }
    Q_EMIT jobRefreshAppstreamMetadataFinished(m_installation, m_remote);
}
