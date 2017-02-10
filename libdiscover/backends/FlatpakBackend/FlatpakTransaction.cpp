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

#include <Transaction/TransactionModel.h>

#include <QDebug>
#include <QTimer>

FlatpakTransaction::FlatpakTransaction(FlatpakInstallation *installation, FlatpakResource *app, Role role)
    : FlatpakTransaction(installation, app, nullptr, {}, role)
{
}

FlatpakTransaction::FlatpakTransaction(FlatpakInstallation* installation, FlatpakResource *app, FlatpakResource *runtime, Transaction::Role role)
    : FlatpakTransaction(installation, app, runtime, {}, role)
{
}

FlatpakTransaction::FlatpakTransaction(FlatpakInstallation *installation, FlatpakResource *app, const AddonList &addons, Transaction::Role role)
    : FlatpakTransaction(installation, app, nullptr, addons, role)
{
}

FlatpakTransaction::FlatpakTransaction(FlatpakInstallation* installation, FlatpakResource *app, FlatpakResource *runtime, const AddonList &list, Transaction::Role role)
    : Transaction(app->backend(), app, role, list)
    , m_appJobFinished(false)
    , m_runtimeJobFinished(false)
    , m_appJobProgress(0)
    , m_runtimeJobProgress(0)
    , m_app(app)
    , m_runtime(runtime)
    , m_installation(installation)
{
    setCancellable(true);

    TransactionModel::global()->addTransaction(this);

    QTimer::singleShot(0, this, &FlatpakTransaction::start);
}

FlatpakTransaction::~FlatpakTransaction()
{
    delete m_appJob;
    delete m_runtimeJob;
}

void FlatpakTransaction::cancel()
{
    m_appJob->cancel();
    if (m_runtime) {
        m_runtimeJob->cancel();
    }
    TransactionModel::global()->cancelTransaction(this);
}

void FlatpakTransaction::start()
{
    if (m_runtime) {
        m_runtimeJob = new FlatpakTransactionJob(m_installation, m_runtime, role());
        connect(m_runtimeJob, &FlatpakTransactionJob::jobFinished, this, &FlatpakTransaction::onRuntimeJobFinished);
        connect(m_runtimeJob, &FlatpakTransactionJob::progressChanged, this, &FlatpakTransaction::onRuntimeJobProgressChanged);
        m_runtimeJob->start();
    } else {
        // We can mark runtime job as finished as we don't need to start it
        m_runtimeJobFinished = true;
    }

    // App job will be started everytime
    m_appJob = new FlatpakTransactionJob(m_installation, m_app, role());
    connect(m_appJob, &FlatpakTransactionJob::jobFinished, this, &FlatpakTransaction::onAppJobFinished);
    connect(m_appJob, &FlatpakTransactionJob::progressChanged, this, &FlatpakTransaction::onAppJobProgressChanged);
    m_appJob->start();
}

void FlatpakTransaction::onAppJobFinished(bool success)
{
    m_appJobFinished = true;
    m_appJobProgress = 100;

    if (m_runtimeJobFinished) {
        finishTransaction(success);
    }
}

void FlatpakTransaction::onAppJobProgressChanged(int progress)
{
    m_appJobProgress = progress;

    updateProgress();
}

void FlatpakTransaction::onRuntimeJobFinished(bool success)
{
    m_runtimeJobFinished = true;
    m_runtimeJobProgress = 100;

    if (m_appJobFinished) {
        finishTransaction(success);
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

void FlatpakTransaction::finishTransaction(bool success)
{
    setStatus(DoneStatus);
    if (success) {
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
        if (m_runtime && role() == InstallRole) {
            m_runtime->setState(newState);
        }
    }
    TransactionModel::global()->removeTransaction(this);
}
