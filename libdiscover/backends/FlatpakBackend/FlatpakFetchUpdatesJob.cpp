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

#include "FlatpakFetchUpdatesJob.h"

#include <QDebug>

FlatpakFetchUpdatesJob::FlatpakFetchUpdatesJob(FlatpakInstallation *installation)
    : QThread()
    , m_installation(installation)
{
    m_cancellable = g_cancellable_new();
}

FlatpakFetchUpdatesJob::~FlatpakFetchUpdatesJob()
{
    g_object_unref(m_cancellable);
}

void FlatpakFetchUpdatesJob::cancel()
{
    g_cancellable_cancel(m_cancellable);
}

void FlatpakFetchUpdatesJob::run()
{
    g_autoptr(GError) localError = nullptr;

    GPtrArray *refs = nullptr;
    refs = flatpak_installation_list_installed_refs_for_update(m_installation, m_cancellable, &localError);
    if (!refs) {
        qWarning() << "Failed to get list of installed refs for listing updates: " << localError->message;
        return;
    }

    Q_EMIT jobFetchUpdatesFinished(m_installation, refs);
}

