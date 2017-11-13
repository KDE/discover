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

    transactionJob->setProgress(progress);

    Q_EMIT transactionJob->progressChanged(progress);
}

FlatpakTransactionJob::FlatpakTransactionJob(FlatpakResource *app, const QPair<QString, uint> &relatedRef, Transaction::Role role, QObject *parent)
    : QThread(parent)
    , m_result(false)
    , m_progress(0)
    , m_relatedRef(relatedRef.first)
    , m_relatedRefKind(relatedRef.second)
    , m_app(app)
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

    const QString refName = m_relatedRef.isEmpty() ? m_app->flatpakName() : m_relatedRef;
    const uint kind = m_relatedRef.isEmpty() ? (uint)m_app->type() : m_relatedRefKind;

    if (m_role == Transaction::Role::InstallRole) {
        bool installRelatedRef = false;
        // Before we attempt to upgrade related refs we should verify whether they are installed in first place
        if (m_app->state() == AbstractResource::Upgradeable && !m_relatedRef.isEmpty()) {
            g_autoptr(GError) installedRefError = nullptr;
            FlatpakInstalledRef *installedRef = flatpak_installation_get_installed_ref(m_app->installation(),
                                                                                       kind == FlatpakResource::DesktopApp ? FLATPAK_REF_KIND_APP : FLATPAK_REF_KIND_RUNTIME,
                                                                                       refName.toUtf8().constData(),
                                                                                       m_app->arch().toUtf8().constData(),
                                                                                       m_app->branch().toUtf8().constData(),
                                                                                       m_cancellable, &installedRefError);
            if (installedRefError) {
                qWarning() << "Failed to check whether related ref is installed: " << installedRefError;
            }
            installRelatedRef = installedRef == nullptr;
        }

        if (m_app->state() == AbstractResource::Upgradeable && !installRelatedRef) {
            ref = flatpak_installation_update(m_app->installation(),
                                              FLATPAK_UPDATE_FLAGS_NONE,
                                              kind == FlatpakResource::DesktopApp ? FLATPAK_REF_KIND_APP : FLATPAK_REF_KIND_RUNTIME,
                                              refName.toUtf8().constData(),
                                              m_app->arch().toUtf8().constData(),
                                              m_app->branch().toUtf8().constData(),
                                              flatpakInstallationProgressCallback,
                                              this,
                                              m_cancellable, &localError);
        } else {
            if (m_app->flatpakFileType() == QStringLiteral("flatpak")) {
                g_autoptr(GFile) file = g_file_new_for_path(m_app->resourceFile().toLocalFile().toUtf8().constData());
                if (!file) {
                    qWarning() << "Failed to install bundled application" << refName;
                }
                ref = flatpak_installation_install_bundle(m_app->installation(), file, flatpakInstallationProgressCallback, this, m_cancellable, &localError);
            } else {
                ref = flatpak_installation_install(m_app->installation(),
                                                   m_app->origin().toUtf8().constData(),
                                                   kind == FlatpakResource::DesktopApp ? FLATPAK_REF_KIND_APP : FLATPAK_REF_KIND_RUNTIME,
                                                   refName.toUtf8().constData(),
                                                   m_app->arch().toUtf8().constData(),
                                                   m_app->branch().toUtf8().constData(),
                                                   flatpakInstallationProgressCallback,
                                                   this,
                                                   m_cancellable, &localError);
            }
        }

        if (!ref) {
            m_result = false;
            m_errorMessage = QString::fromUtf8(localError->message);
            // We are done so we can set the progress to 100
            m_progress = 100;
            qWarning() << "Failed to install" << refName << ':' << m_errorMessage;
            return;
        }
    } else if (m_role == Transaction::Role::RemoveRole) {
        if (!flatpak_installation_uninstall(m_app->installation(),
                                            kind == FlatpakResource::DesktopApp ? FLATPAK_REF_KIND_APP : FLATPAK_REF_KIND_RUNTIME,
                                            refName.toUtf8().constData(),
                                            m_app->arch().toUtf8().constData(),
                                            m_app->branch().toUtf8().constData(),
                                            flatpakInstallationProgressCallback,
                                            this,
                                            m_cancellable, &localError)) {
            m_result = false;
            m_errorMessage = QString::fromUtf8(localError->message);
            // We are done so we can set the progress to 100
            m_progress = 100;
            qWarning() << "Failed to uninstall" << refName << ':' << m_errorMessage;
            return;
        }
    }

    // We are done so we can set the progress to 100
    m_progress = 100;
    m_result = true;

    Q_EMIT progressChanged(m_progress);
}

FlatpakResource * FlatpakTransactionJob::app() const
{
    return m_app;
}

bool FlatpakTransactionJob::isRelated() const
{
    return !m_relatedRef.isEmpty();
}

int FlatpakTransactionJob::progress() const
{
    return m_progress;
}

void FlatpakTransactionJob::setProgress(int progress)
{
    m_progress = progress;
}

QString FlatpakTransactionJob::errorMessage() const
{
    return m_errorMessage;
}

bool FlatpakTransactionJob::result() const
{
    return m_result;
}

