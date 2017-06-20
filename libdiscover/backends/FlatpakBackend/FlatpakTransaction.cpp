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

FlatpakTransaction::FlatpakTransaction(FlatpakInstallation *installation, FlatpakResource *app, Role role, bool delayStart)
    : FlatpakTransaction(installation, app, nullptr, role, delayStart)
{
}

FlatpakTransaction::FlatpakTransaction(FlatpakInstallation* installation, FlatpakResource *app, FlatpakResource *runtime, Transaction::Role role, bool delayStart)
    : Transaction(app->backend(), app, role, {})
    , m_appJobProgress(0)
    , m_runtimeJobProgress(0)
    , m_app(app)
    , m_runtime(runtime)
    , m_installation(installation)
{
    setCancellable(true);

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
    m_appJob->cancel();
    if (m_runtime) {
        m_runtimeJob->cancel();
    }
    setStatus(CancelledStatus);
}

void FlatpakTransaction::setRuntime(FlatpakResource *runtime)
{
    m_runtime = runtime;
}

void FlatpakTransaction::start()
{
    if (m_runtime) {
        m_runtimeJob = new FlatpakTransactionJob(m_installation, m_runtime, role(), this);
        connect(m_runtimeJob, &FlatpakTransactionJob::finished, this, &FlatpakTransaction::onRuntimeJobFinished);
        connect(m_runtimeJob, &FlatpakTransactionJob::progressChanged, this, &FlatpakTransaction::onRuntimeJobProgressChanged);
        m_runtimeJob->start();
    }

    // App job will be started everytime
    m_appJob = new FlatpakTransactionJob(m_installation, m_app, role(), this);
    connect(m_appJob, &FlatpakTransactionJob::finished, this, &FlatpakTransaction::onAppJobFinished);
    connect(m_appJob, &FlatpakTransactionJob::progressChanged, this, &FlatpakTransaction::onAppJobProgressChanged);
    m_appJob->start();
}

void FlatpakTransaction::onAppJobFinished()
{
    m_appJobProgress = 100;

    updateProgress();

    if (!m_appJob->result()) {
        Q_EMIT passiveMessage(m_appJob->errorMessage());
    }

    if ((m_runtimeJob && m_runtimeJob->isFinished()) || !m_runtimeJob) {
        finishTransaction();
    }
}

void FlatpakTransaction::onAppJobProgressChanged(int progress)
{
    m_appJobProgress = progress;

    updateProgress();
}

void FlatpakTransaction::onRuntimeJobFinished()
{
    m_runtimeJobProgress = 100;

    updateProgress();

    if (!m_runtimeJob->result()) {
        Q_EMIT passiveMessage(m_runtimeJob->errorMessage());
    } else {
        // This should be the only case when runtime is automatically installed, but better to check
        if (role() == InstallRole) {
            m_runtime->setState(AbstractResource::Installed);
        }
    }

    if (m_appJob->isFinished()) {
        finishTransaction();
    }
}

void FlatpakTransaction::onRuntimeJobProgressChanged(int progress)
{
    m_runtimeJobProgress = progress;

    updateProgress();
}

void FlatpakTransaction::updateProgress()
{
    if (m_runtime) {
        setProgress((m_appJobProgress + m_runtimeJobProgress) / 2);
    } else {
        setProgress(m_appJobProgress);
    }
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
