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

#include "FlatpakTransactionJob.h"
#include "FlatpakResource.h"

#include <QDebug>

static void flatpakInstallationProgressCallback(const gchar *stats, guint progress, gboolean estimating, gpointer userData)
{
    Q_UNUSED(estimating);
    Q_UNUSED(stats);

    FlatpakTransactionJob *transactionJob = (FlatpakTransactionJob*)userData;
    if (!transactionJob) {
        return;
    }

    Q_EMIT transactionJob->progressChanged(progress);
}

FlatpakTransactionJob::FlatpakTransactionJob(FlatpakInstallation *installation, FlatpakResource *app, Transaction::Role role)
    : QThread()
    , m_app(app)
    , m_installation(installation)
    , m_role(role)
{
    m_cancellable = g_cancellable_new();
}

void FlatpakTransactionJob::cancel()
{
    g_cancellable_cancel(m_cancellable);
}

void FlatpakTransactionJob::run()
{
    g_autoptr(GError) localError = nullptr;
    g_autoptr(GCancellable) cancellable = g_cancellable_new();
    g_autoptr(FlatpakInstalledRef) ref = nullptr;

    if (m_role == Transaction::Role::InstallRole) {
        ref = flatpak_installation_install(m_installation,
                                           m_app->origin().toStdString().c_str(),
                                           m_app->flatpakRefKind(),
                                           m_app->flatpakName().toStdString().c_str(),
                                           m_app->arch().toStdString().c_str(),
                                           m_app->branch().toStdString().c_str(),
                                           flatpakInstallationProgressCallback,
                                           this,
                                           cancellable, &localError);
        if (!ref) {
            qWarning() << "Failed to install " << m_app->name() << " :"<< localError->message;
            Q_EMIT jobFinished(false);
        }
    } else if (m_role == Transaction::Role::RemoveRole) {
        if (!flatpak_installation_uninstall(m_installation,
                                            m_app->flatpakRefKind(),
                                            m_app->flatpakName().toStdString().c_str(),
                                            m_app->arch().toStdString().c_str(),
                                            m_app->branch().toStdString().c_str(),
                                            flatpakInstallationProgressCallback,
                                            this,
                                            cancellable, &localError)) {
            qWarning() << "Failed to uninstall " << m_app->name() << " :" << localError->message;
            Q_EMIT jobFinished(false);
        }
    }

    Q_EMIT jobFinished(true);
}
