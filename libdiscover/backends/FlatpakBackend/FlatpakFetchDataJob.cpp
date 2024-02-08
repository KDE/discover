/*
 *   SPDX-FileCopyrightText: 2017 Jan Grulich <jgrulich@redhat.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "FlatpakFetchDataJob.h"
#include "FlatpakResource.h"
#include "libdiscover_backend_flatpak_debug.h"

namespace FlatpakRunnables
{
FlatpakRemoteRef *findRemoteRef(FlatpakResource *app, GCancellable *cancellable)
{
    if (app->origin().isEmpty()) {
        qCWarning(LIBDISCOVER_BACKEND_FLATPAK_LOG) << "Failed to get metadata file because of missing origin";
        return nullptr;
    }

    g_autoptr(GError) localError = nullptr;
    const auto kind = app->resourceType() == FlatpakResource::DesktopApp ? FLATPAK_REF_KIND_APP : FLATPAK_REF_KIND_RUNTIME;
    const QByteArray origin = app->origin().toUtf8(), name = app->flatpakName().toUtf8(), arch = app->arch().toUtf8(), branch = app->branch().toUtf8();
    auto ret = flatpak_installation_fetch_remote_ref_sync_full(app->installation(),
                                                               origin.constData(),
                                                               kind,
                                                               name.constData(),
                                                               arch.constData(),
                                                               branch.constData(),
                                                               FLATPAK_QUERY_FLAGS_ONLY_CACHED,
                                                               cancellable,
                                                               &localError);
    if (localError) {
        qCWarning(LIBDISCOVER_BACKEND_FLATPAK_LOG) << "Failed to find remote ref:" << localError->message;
    }
    return ret;
}

QByteArray fetchMetadata(FlatpakResource *app, GCancellable *cancellable)
{
    FlatpakRemoteRef *remoteRef = findRemoteRef(app, cancellable);
    if (!remoteRef) {
        if (!g_cancellable_is_cancelled(cancellable)) {
            qCDebug(LIBDISCOVER_BACKEND_FLATPAK_LOG) << "failed to find the remote" << app->name();
        }

        return {};
    }

    g_autoptr(GBytes) data = flatpak_remote_ref_get_metadata(remoteRef);
    gsize len = 0;
    auto buff = g_bytes_get_data(data, &len);
    const QByteArray metadataContent((const char *)buff, len);

    if (metadataContent.isEmpty()) {
        qCWarning(LIBDISCOVER_BACKEND_FLATPAK_LOG) << "Failed to get metadata file: empty metadata";
        return {};
    }
    return metadataContent;
}

}
