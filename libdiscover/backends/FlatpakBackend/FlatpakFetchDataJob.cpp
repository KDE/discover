/*
 *   SPDX-FileCopyrightText: 2017 Jan Grulich <jgrulich@redhat.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */


#include "FlatpakFetchDataJob.h"
#include "FlatpakResource.h"

namespace FlatpakRunnables
{

static FlatpakRemoteRef* findRemoteRef(FlatpakResource *app, GCancellable* cancellable, GError **error)
{
    if (app->origin().isEmpty()) {
        qWarning() << "Failed to get metadata file because of missing origin";
        return nullptr;
    }

    const auto kind = app->resourceType() == FlatpakResource::DesktopApp ? FLATPAK_REF_KIND_APP : FLATPAK_REF_KIND_RUNTIME;
    const QByteArray origin = app->origin().toUtf8(), name = app->flatpakName().toUtf8(), arch = app->arch().toUtf8(), branch = app->branch().toUtf8();
    return flatpak_installation_fetch_remote_ref_sync_full(app->installation(), origin.constData(), kind, name.constData(), arch.constData(), branch.constData(), FLATPAK_QUERY_FLAGS_ONLY_CACHED, cancellable, error);
}

QByteArray fetchMetadata(FlatpakResource *app, GCancellable* cancellable)
{
    g_autoptr(GError) localError = nullptr;

    FlatpakRemoteRef* remote = findRemoteRef(app, cancellable, &localError);

    Q_ASSERT(remote);
    Q_ASSERT(!localError);

    QByteArray metadataContent;
    g_autoptr(GBytes) data = flatpak_remote_ref_get_metadata(remote);
    if (data) {
        gsize len = 0;
        auto buff = g_bytes_get_data(data, &len);
        metadataContent = QByteArray((const char*) buff, len);
    } else {
        qWarning() << "Failed to get metadata file: " << localError->message;
        return {};
    }

    if (metadataContent.isEmpty()) {
        qWarning() << "Failed to get metadata file: empty metadata";
        return {};
    }

    return metadataContent;
}

SizeInformation fetchFlatpakSize(FlatpakResource *app, GCancellable* cancellable)
{
    g_autoptr(GError) localError = nullptr;

    SizeInformation ret;
    FlatpakRemoteRef* remote = findRemoteRef(app, cancellable, &localError);
    if (!remote) {
        qWarning() << "Failed to get remote size of" << app->name() << app->origin() << ":" << localError->message;
        return ret;
    }

    ret.downloadSize = flatpak_remote_ref_get_download_size(remote);
    ret.installedSize = flatpak_remote_ref_get_installed_size(remote);
    ret.valid = true;
    return ret;
}

}
