/***************************************************************************
 *   Copyright © 2013 Aleix Pol Gonzalez <aleixpol@blue-systems.com>       *
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

#include "FlatpakTransaction.h"
#include "FlatpakBackend.h"
#include "FlatpakResource.h"
#include "FlatpakTransactionJob.h"

#include <QDebug>
#include <QTimer>

extern "C" {
#include <flatpak.h>
#include <gio/gio.h>
#include <glib.h>
}

FlatpakTransaction::FlatpakTransaction(FlatpakResource *app, Role role, bool delayStart)
    : FlatpakTransaction(app, nullptr, role, delayStart)
{
}

FlatpakTransaction::FlatpakTransaction(FlatpakResource *app, FlatpakResource *runtime, Transaction::Role role, bool delayStart)
    : Transaction(app->backend(), app, role, {})
    , m_app(app)
    , m_runtime(runtime)
{
    setCancellable(true);
    setStatus(QueuedStatus);

    if (!delayStart) {
        QTimer::singleShot(0, this, &FlatpakTransaction::start);
    }
}

FlatpakTransaction::~FlatpakTransaction()
{
}

void FlatpakTransaction::cancel()
{
    Q_ASSERT(m_appJob);
    foreach (const QPointer<FlatpakTransactionJob> &job, m_jobs) {
        job->cancel();
    }
    setStatus(CancelledStatus);
}

void FlatpakTransaction::setRuntime(FlatpakResource *runtime)
{
    m_runtime = runtime;
}

void FlatpakTransaction::start()
{
    setStatus(DownloadingStatus);
    if (m_runtime) {
        QPointer<FlatpakTransactionJob> job = new FlatpakTransactionJob(m_runtime, QPair<QString, uint>(), role(), this);
        connect(job, &FlatpakTransactionJob::finished, this, &FlatpakTransaction::onJobFinished);
        connect(job, &FlatpakTransactionJob::progressChanged, this, &FlatpakTransaction::onJobProgressChanged);
        m_jobs << job;

        processRelatedRefs(m_runtime);
    }

    // App job will be added everytime
    m_appJob = new FlatpakTransactionJob(m_app, QPair<QString, uint>(), role(), this);
    connect(m_appJob, &FlatpakTransactionJob::finished, this, &FlatpakTransaction::onJobFinished);
    connect(m_appJob, &FlatpakTransactionJob::progressChanged, this, &FlatpakTransaction::onJobProgressChanged);
    m_jobs << m_appJob;

    processRelatedRefs(m_app);


    // Now start all the jobs together
    foreach (const QPointer<FlatpakTransactionJob> &job, m_jobs) {
        job->start();
    }
}

void FlatpakTransaction::processRelatedRefs(FlatpakResource* resource)
{
    g_autoptr(GPtrArray) refs = nullptr;
    g_autoptr(GError) error = nullptr;
    g_autoptr(GCancellable) cancellable = g_cancellable_new();;
    QList<FlatpakResource> additionalResources;

    g_autofree gchar *ref = nullptr;
    ref = g_strdup_printf ("%s/%s/%s/%s",
                           resource->typeAsString().toUtf8().constData(),
                           resource->flatpakName().toUtf8().constData(),
                           resource->arch().toUtf8().constData(),
                           resource->branch().toUtf8().constData());

    if (role() == Transaction::Role::InstallRole) {
        if (resource->state() == AbstractResource::Upgradeable) {
            refs = flatpak_installation_list_installed_related_refs_sync(resource->installation(), resource->origin().toUtf8().constData(), ref, cancellable, &error);
            if (error) {
                qWarning() << "Failed to list installed related refs for update: " << error->message;
            }
        } else {
            refs = flatpak_installation_list_remote_related_refs_sync(resource->installation(), resource->origin().toUtf8().constData(), ref, cancellable, &error);
            if (error) {
                qWarning() << "Failed to list related refs for installation: " << error->message;
            }
        }
    } else if (role() == Transaction::Role::RemoveRole) {
        refs = flatpak_installation_list_installed_related_refs_sync(resource->installation(), resource->origin().toUtf8().constData(), ref, cancellable, &error);
        if (error) {
            qWarning() << "Failed to list installed related refs for removal: " << error->message;
        }
    }

    if (refs) {
        for (uint i = 0; i < refs->len; i++) {
            FlatpakRef *flatpakRef = FLATPAK_REF(g_ptr_array_index(refs, i));
            if (flatpak_related_ref_should_download(FLATPAK_RELATED_REF(flatpakRef))) {
                QPointer<FlatpakTransactionJob> job = new FlatpakTransactionJob(resource, QPair<QString, uint>(QString::fromUtf8(flatpak_ref_get_name(flatpakRef)), flatpak_ref_get_kind(flatpakRef)), role(), this);
                connect(job, &FlatpakTransactionJob::finished, this, &FlatpakTransaction::onJobFinished);
                connect(job, &FlatpakTransactionJob::progressChanged, this, &FlatpakTransaction::onJobProgressChanged);
                // Add to the list of all jobs
                m_jobs << job;
            }
        }
    }
}

void FlatpakTransaction::onJobFinished()
{
    FlatpakTransactionJob *job = static_cast<FlatpakTransactionJob*>(sender());

    if (job != m_appJob) {
        if (!job->result()) {
            Q_EMIT passiveMessage(job->errorMessage());
        }

        // Mark runtime as installed
        if (m_runtime && job->app()->flatpakName() == m_runtime->flatpakName() && !job->isRelated() && role() != Transaction::Role::RemoveRole) {
            if (job->result()) {
                m_runtime->setState(AbstractResource::Installed);
            }
        }
    }

    foreach (const QPointer<FlatpakTransactionJob> &job, m_jobs) {
        if (job->isRunning()) {
            return;
        }
    }

    // No other job is running → finish transaction
    finishTransaction();
}

void FlatpakTransaction::onJobProgressChanged(int progress)
{
    Q_UNUSED(progress);

    int total = 0;

    // Count progress from all the jobs
    foreach (const QPointer<FlatpakTransactionJob> &job, m_jobs) {
        total += job->progress();
    }

    setProgress(total / m_jobs.count());
}

void FlatpakTransaction::finishTransaction()
{
    if (m_appJob->result()) {
        AbstractResource::State newState = AbstractResource::None;
        switch(role()) {
        case InstallRole:
        case ChangeAddonsRole:
            newState = AbstractResource::Installed;
            break;
        case RemoveRole:
            newState = AbstractResource::None;
            break;
        }

        m_app->setState(newState);

        setStatus(DoneStatus);
    } else {
        setStatus(DoneWithErrorStatus);
    }
}
