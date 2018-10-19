/***************************************************************************
 *   Copyright © 2017 Jan Grulich <jgrulich@redhat.com>                    *
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
QByteArray fetchMetadata(FlatpakInstallation *installation, FlatpakResource *app)
{
    g_autoptr(GCancellable) cancellable = g_cancellable_new();
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
    g_autoptr(GBytes) data = flatpak_installation_fetch_remote_metadata_sync(installation, app->origin().toUtf8().constData(), fakeRef, cancellable, &localError);
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

SizeInformation fetchFlatpakSize(FlatpakInstallation *installation, FlatpakResource *app)
{
    g_autoptr(GCancellable) cancellable = g_cancellable_new();
    g_autoptr(GError) localError = nullptr;

    SizeInformation ret;
    g_autoptr(FlatpakRef) ref = createFakeRef(app);
    if (!ref) {
        return ret;
    }

    if (!flatpak_installation_fetch_remote_size_sync(installation, app->origin().toUtf8().constData(), ref, &ret.downloadSize, &ret.installedSize, cancellable, &localError)) {
        qWarning() << "Failed to get remote size of " << app->name() << ": " << localError->message;
        return ret;
    }

    ret.valid = true;
    return ret;
}

}
