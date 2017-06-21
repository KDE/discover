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

FlatpakTransactionJob::FlatpakTransactionJob(FlatpakInstallation *installation, FlatpakResource *app, Transaction::Role role, QObject *parent)
    : QThread(parent)
    , m_result(false)
    , m_app(app)
    , m_installation(installation)
    , m_role(role)
{
    m_cancellable = g_cancellable_new();
}

FlatpakTransactionJob::~FlatpakTransactionJob()
{
    g_object_unref(m_cancellable);
}

void FlatpakTransactionJob::cancel()
{
    g_cancellable_cancel(m_cancellable);
}

void FlatpakTransactionJob::run()
{
    g_autoptr(GError) localError = nullptr;
    g_autoptr(FlatpakInstalledRef) ref = nullptr;

    if (m_role == Transaction::Role::InstallRole) {
        if (m_app->state() == AbstractResource::Upgradeable) {
            ref = flatpak_installation_update(m_installation,
                                              FLATPAK_UPDATE_FLAGS_NONE,
                                              m_app->type() == FlatpakResource::DesktopApp ? FLATPAK_REF_KIND_APP : FLATPAK_REF_KIND_RUNTIME,
                                              m_app->flatpakName().toStdString().c_str(),
                                              m_app->arch().toStdString().c_str(),
                                              m_app->branch().toStdString().c_str(),
                                              flatpakInstallationProgressCallback,
                                              this,
                                              m_cancellable, &localError);
        } else {
            if (m_app->flatpakFileType() == QStringLiteral("flatpak")) {
                g_autoptr(GFile) file = g_file_new_for_path(m_app->resourceFile().toLocalFile().toStdString().c_str());
                if (!file) {
                    qWarning() << "Failed to install bundled application" << m_app->name();
                }
                ref = flatpak_installation_install_bundle(m_installation, file, flatpakInstallationProgressCallback, this, m_cancellable, &localError);
            } else {
                ref = flatpak_installation_install(m_installation,
                                                m_app->origin().toStdString().c_str(),
                                                m_app->type() == FlatpakResource::DesktopApp ? FLATPAK_REF_KIND_APP : FLATPAK_REF_KIND_RUNTIME,
                                                m_app->flatpakName().toStdString().c_str(),
                                                m_app->arch().toStdString().c_str(),
                                                m_app->branch().toStdString().c_str(),
                                                flatpakInstallationProgressCallback,
                                                this,
                                                m_cancellable, &localError);
            }
        }

        if (!ref) {
            m_result = false;
            m_errorMessage = QString::fromUtf8(localError->message);
            qWarning() << "Failed to install" << m_app->name() << ':' << m_errorMessage;
            return;
        }
    } else if (m_role == Transaction::Role::RemoveRole) {
        if (!flatpak_installation_uninstall(m_installation,
                                            m_app->type() == FlatpakResource::DesktopApp ? FLATPAK_REF_KIND_APP : FLATPAK_REF_KIND_RUNTIME,
                                            m_app->flatpakName().toStdString().c_str(),
                                            m_app->arch().toStdString().c_str(),
                                            m_app->branch().toStdString().c_str(),
                                            flatpakInstallationProgressCallback,
                                            this,
                                            m_cancellable, &localError)) {
            m_result = false;
            m_errorMessage = QString::fromUtf8(localError->message);
            qWarning() << "Failed to uninstall" << m_app->name() << ':' << m_errorMessage;
            return;
        }
    }

    m_result = true;
}

QString FlatpakTransactionJob::errorMessage() const
{
    return m_errorMessage;
}

bool FlatpakTransactionJob::result() const
{
    return m_result;
}

