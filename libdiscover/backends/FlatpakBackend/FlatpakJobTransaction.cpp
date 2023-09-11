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
        constexpr auto arbitraryMaxConcurrency = 4U;
        const auto concurrency = std::min(std::thread::hardware_concurrency(), arbitraryMaxConcurrency);
        setMaxThreadCount(std::make_signed_t<decltype(concurrency)>(concurrency));
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
    setStatus(DownloadingStatus);

    // App job will be added every time
    m_appJob = new FlatpakTransactionThread(m_app, role());
    m_appJob->setAutoDelete(false);
    connect(m_appJob, &FlatpakTransactionThread::finished, this, &FlatpakJobTransaction::finishTransaction);
    connect(m_appJob, &FlatpakTransactionThread::progressChanged, this, &FlatpakJobTransaction::setProgress);
    connect(m_appJob, &FlatpakTransactionThread::speedChanged, this, &FlatpakJobTransaction::setDownloadSpeed);
    connect(m_appJob, &FlatpakTransactionThread::passiveMessage, this, &FlatpakJobTransaction::passiveMessage);
    connect(m_appJob, &FlatpakTransactionThread::webflowStarted, this, &FlatpakJobTransaction::webflowStarted);
    connect(m_appJob, &FlatpakTransactionThread::webflowDone, this, &FlatpakJobTransaction::webflowDone);

    s_pool->start(m_appJob);
}

void FlatpakJobTransaction::finishTransaction()
{
    if (static_cast<FlatpakBackend *>(m_app->backend())->getInstalledRefForApp(m_app)) {
        m_app->setState(AbstractResource::Installed);
    } else {
        m_app->setState(AbstractResource::None);
    }

    if (!m_appJob->addedRepositories().isEmpty()) {
        Q_EMIT repositoriesAdded(m_appJob->addedRepositories());
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
