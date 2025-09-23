/*
 *   SPDX-FileCopyrightText: 2013 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *   SPDX-FileCopyrightText: 2017 Jan Grulich <jgrulich@redhat.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "FlatpakRefreshAppstreamMetadataJob.h"
#include "libdiscover_backend_flatpak_debug.h"

FlatpakRefreshAppstreamMetadataJob::FlatpakRefreshAppstreamMetadataJob(FlatpakInstallation *installation, FlatpakRemote *remote)
    : QThread()
    , m_cancellable(g_cancellable_new())
    , m_installation(g_object_ref(installation))
    , m_remote(g_object_ref(remote))
{
    connect(this, &FlatpakRefreshAppstreamMetadataJob::finished, this, &QObject::deleteLater);
}

FlatpakRefreshAppstreamMetadataJob::~FlatpakRefreshAppstreamMetadataJob()
{
    g_object_unref(m_cancellable);
    g_object_unref(m_installation);
    g_object_unref(m_remote);
}

void FlatpakRefreshAppstreamMetadataJob::cancel()
{
    g_cancellable_cancel(m_cancellable);
}

void FlatpakRefreshAppstreamMetadataJob::updateCallback(const char *status, guint progress, gboolean estimating, gpointer user_data)
{
    qCDebug(LIBDISCOVER_BACKEND_FLATPAK_LOG) << "updating metadata..." << status;

    auto self = static_cast<FlatpakRefreshAppstreamMetadataJob *>(user_data);
    self->m_progress = progress;
    self->m_estimating = estimating;
    Q_EMIT self->progressChanged();
}

void FlatpakRefreshAppstreamMetadataJob::run()
{
    g_autoptr(GError) localError = nullptr;

    gboolean changed = false;
    if (!flatpak_installation_update_appstream_full_sync(m_installation,
                                                         flatpak_remote_get_name(m_remote),
                                                         nullptr,
                                                         &FlatpakRefreshAppstreamMetadataJob::updateCallback,
                                                         this,
                                                         &changed,
                                                         m_cancellable,
                                                         &localError)) {
        const QString error = localError ? QString::fromUtf8(localError->message) : QStringLiteral("<no error>");
        qCWarning(LIBDISCOVER_BACKEND_FLATPAK_LOG).nospace()
            << "Failed to refresh appstream metadata for " << flatpak_remote_get_name(m_remote) << ": " << error;
    }
    Q_EMIT jobRefreshAppstreamMetadataFinished(m_installation, m_remote);
}

#include "moc_FlatpakRefreshAppstreamMetadataJob.cpp"
