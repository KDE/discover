/*
 *   SPDX-FileCopyrightText: 2013 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *   SPDX-FileCopyrightText: 2017 Jan Grulich <jgrulich@redhat.com>
 *   SPDX-FileCopyrightText: 2023 Harald Sitter <sitter@kde.org>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "FlatpakJobTransaction.h"
#include "FlatpakBackend.h"
#include "FlatpakResource.h"
#include "FlatpakTransactionThread.h"

#include <thread>

#include <QDebug>
#include <QScopeGuard>
#include <QTimer>

namespace
{
class ThreadPool : public QThreadPool
{
public:
    ThreadPool()
    {
        // Cap the amount of concurrency to prevent too many in-flight transactions. This in particular
        // prevents running out of file descriptors or other limited resources.
        // https://bugs.kde.org/show_bug.cgi?id=474231
        // Additionally we cannot go above 1 because it'd break serialization expectations when auth hardening is on.
        // https://bugs.kde.org/show_bug.cgi?id=466559
        // Note that threading is still worthwhile here because we may need to block the transactions while doing
        // GUI interaction with the user.
        setMaxThreadCount(1);
    }
};
} // namespace

Q_GLOBAL_STATIC(ThreadPool, s_pool);

FlatpakJobTransaction::FlatpakJobTransaction(FlatpakResource *app, Role role, bool delayStart)
    : Transaction(app->backend(), app, role, {})
    , m_app(app)
{
    setCancellable(true);
    setStatus(QueuedStatus);

    if (!delayStart) {
        QTimer::singleShot(0, this, &FlatpakJobTransaction::start);
    }
}

FlatpakJobTransaction::~FlatpakJobTransaction()
{
    cancel();
    if (s_pool->tryTake(m_appJob)) { // immediately delete if the runnable hasn't started yet
        delete m_appJob;
    } else { // otherwise defer cleanup to the pool
        m_appJob->setAutoDelete(true);
    }
}

void FlatpakJobTransaction::cancel()
{
    m_appJob->cancel();
}

void FlatpakJobTransaction::start()
{
    setStatus(CommittingStatus);

    // App job will be added every time
    m_appJob = new FlatpakTransactionThread(m_app, role());
    m_appJob->setAutoDelete(false);
    connect(m_appJob, &FlatpakTransactionThread::finished, this, &FlatpakJobTransaction::finishTransaction);
    connect(m_appJob, &FlatpakTransactionThread::progressChanged, this, &FlatpakJobTransaction::setProgress);
    connect(m_appJob, &FlatpakTransactionThread::speedChanged, this, &FlatpakJobTransaction::setDownloadSpeed);
    connect(m_appJob, &FlatpakTransactionThread::passiveMessage, this, &FlatpakJobTransaction::passiveMessage);
    connect(m_appJob, &FlatpakTransactionThread::webflowStarted, this, &FlatpakJobTransaction::webflowStarted);
    connect(m_appJob, &FlatpakTransactionThread::webflowDone, this, &FlatpakJobTransaction::webflowDone);
    connect(m_appJob, &FlatpakTransactionThread::proceedRequest, this, &FlatpakJobTransaction::proceedRequest);

    s_pool->start(m_appJob);
}

void FlatpakJobTransaction::finishTransaction()
{
    if (static_cast<FlatpakBackend *>(m_app->backend())->getInstalledRefForApp(m_app)) {
        m_app->setState(AbstractResource::Installed);
    } else {
        m_app->setState(AbstractResource::None);
    }

    if (const auto repositories = m_appJob->addedRepositories(); !repositories.isEmpty()) {
        Q_EMIT repositoriesAdded(repositories);
    }

    if (!m_appJob->cancelled() && !m_appJob->errorMessage().isEmpty()) {
        Q_EMIT passiveMessage(m_appJob->errorMessage());
    }

    if (m_appJob->result()) {
        setStatus(DoneStatus);
    } else {
        setStatus(m_appJob->cancelled() ? CancelledStatus : DoneWithErrorStatus);
    }
}

void FlatpakJobTransaction::proceed()
{
    if (m_appJob) {
        m_appJob->proceed();
    }
}

#include "moc_FlatpakJobTransaction.cpp"
