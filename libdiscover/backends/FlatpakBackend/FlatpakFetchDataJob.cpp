/*
 *   SPDX-FileCopyrightText: 2017 Jan Grulich <jgrulich@redhat.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */


#include "FlatpakFetchDataJob.h"
#include "FlatpakResource.h"

#include <QDebug>

static FlatpakRef * createFakeRef(FlatpakResource *resource)
{
    FlatpakRef *ref = nullptr;
    g_autoptr(GError) localError = nullptr;

    const auto id = resource->ref();
    ref = flatpak_ref_parse(id.toUtf8().constData(), &localError);

    if (!ref) {
        qWarning() << "Failed to create fake ref: " << localError->message;
    }

    return ref;
}

namespace FlatpakRunnables
{
QByteArray fetchMetadata(FlatpakResource *app, GCancellable* cancellable)
{
    g_autoptr(GError) localError = nullptr;

    if (app->origin().isEmpty()) {
        qWarning() << "Failed to get metadata file because of missing origin";
        return {};
    }

    g_autoptr(FlatpakRef) fakeRef = createFakeRef(app);
    if (!fakeRef) {
        return {};
    }

    QByteArray metadataContent;
    g_autoptr(GBytes) data = flatpak_installation_fetch_remote_metadata_sync(app->installation(), app->origin().toUtf8().constData(), fakeRef, cancellable, &localError);
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
    g_autoptr(FlatpakRef) ref = createFakeRef(app);
    if (!ref) {
        return ret;
    }

    if (!flatpak_installation_fetch_remote_size_sync(app->installation(), app->origin().toUtf8().constData(), ref, &ret.downloadSize, &ret.installedSize, cancellable, &localError)) {
        qWarning() << "Failed to get remote size of " << app->name() << ": " << localError->message;
        return ret;
    }

    ret.valid = true;
    return ret;
}

}
