/*
 *   SPDX-FileCopyrightText: 2013 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *   SPDX-FileCopyrightText: 2017 Jan Grulich <jgrulich@redhat.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "FlatpakJobTransaction.h"
#include "FlatpakBackend.h"
#include "FlatpakResource.h"
#include "FlatpakTransactionThread.h"

#include <QDebug>
#include <QTimer>

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
    if (m_appJob->isRunning()) {
        cancel();
        m_appJob->wait();
    }
    delete m_appJob;
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
    connect(m_appJob, &FlatpakTransactionThread::finished, this, &FlatpakJobTransaction::finishTransaction);
    connect(m_appJob, &FlatpakTransactionThread::progressChanged, this, &FlatpakJobTransaction::setProgress);
    connect(m_appJob, &FlatpakTransactionThread::speedChanged, this, &FlatpakJobTransaction::setDownloadSpeed);
    connect(m_appJob, &FlatpakTransactionThread::passiveMessage, this, &FlatpakJobTransaction::passiveMessage);

    m_appJob->start();
}

void FlatpakJobTransaction::finishTransaction()
{
    if (static_cast<FlatpakBackend *>(m_app->backend())->getInstalledRefForApp(m_app)) {
        m_app->setState(AbstractResource::Installed);
    } else {
        m_app->setState(AbstractResource::None);
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
