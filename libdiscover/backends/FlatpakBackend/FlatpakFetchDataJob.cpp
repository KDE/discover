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

FlatpakFetchDataJob::FlatpakFetchDataJob(FlatpakInstallation *installation, FlatpakResource *app, FlatpakFetchDataJob::DataKind kind)
    : QThread()
    , m_app(app)
    , m_installation(installation)
    , m_kind(kind)
{
    m_cancellable = g_cancellable_new();
}

FlatpakFetchDataJob::~FlatpakFetchDataJob()
{
    g_object_unref(m_cancellable);
}

void FlatpakFetchDataJob::cancel()
{
    g_cancellable_cancel(m_cancellable);
}

void FlatpakFetchDataJob::run()
{
    g_autoptr(GError) localError = nullptr;

    if (m_kind == FetchMetadata) {
        QByteArray metadataContent;
        g_autoptr(GBytes) data = nullptr;
        g_autoptr(FlatpakRef) fakeRef = nullptr;

        if (m_app->origin().isEmpty()) {
            qWarning() << "Failed to get metadata file because of missing origin";
            return;
        }

        fakeRef = createFakeRef(m_app);
        if (!fakeRef) {
            return;
        }

        data = flatpak_installation_fetch_remote_metadata_sync(m_installation, m_app->origin().toStdString().c_str(), fakeRef, m_cancellable, &localError);
        if (data) {
            gsize len = 0;
            metadataContent = QByteArray((char *)g_bytes_get_data(data, &len));
        } else {
            qWarning() << "Failed to get metadata file: " << localError->message;
            return;
        }

        if (metadataContent.isEmpty()) {
            qWarning() << "Failed to get metadata file: empty metadata";
            return;
        }

        Q_EMIT jobFetchMetadataFinished(m_installation, m_app, metadataContent);
    } else if (m_kind == FetchSize) {
        guint64 downloadSize = 0;
        guint64 installedSize = 0;
        g_autoptr(FlatpakRef) ref = nullptr;

        ref = createFakeRef(m_app);
        if (!ref) {
            return;
        }

        if (!flatpak_installation_fetch_remote_size_sync(m_installation, m_app->origin().toStdString().c_str(),
                                                         ref, &downloadSize, &installedSize, m_cancellable, &localError)) {
            qWarning() << "Failed to get remote size of " << m_app->name() << ": " << localError->message;
            return;
        }

        Q_EMIT jobFetchSizeFinished(m_app, downloadSize, installedSize);
    }
}

FlatpakRef * FlatpakFetchDataJob::createFakeRef(FlatpakResource *resource)
{
    FlatpakRef *ref = nullptr;
    g_autoptr(GError) localError = nullptr;

    const QString id = QString::fromUtf8("%1/%2/%3/%4").arg(resource->typeAsString()).arg(resource->flatpakName()).arg(resource->arch()).arg(resource->branch());
    ref = flatpak_ref_parse(id.toStdString().c_str(), &localError);

    if (!ref) {
        qWarning() << "Failed to create fake ref: " << localError->message;
    }

    return ref;
}
