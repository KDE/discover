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

static QHash<FlatpakInstallation *, QSharedPointer<FlatpakTransactionThread>> s_pending;

QSharedPointer<FlatpakTransactionThread> jobForInstallation(FlatpakInstallation *installation)
{
    auto &inst = s_pending[installation];
    if (!inst) {
        inst = QSharedPointer<FlatpakTransactionThread>::create(installation);
    }
    return inst;
}

FlatpakJobTransaction::FlatpakJobTransaction(FlatpakResource *app, Role role)
    : Transaction(app->backend(), app, role, {})
    , m_app(app)
    , m_appJob(jobForInstallation(app->installation()))
{
    setCancellable(true);
    setStatus(QueuedStatus);
    m_appJob->add(m_app, role);
    connect(m_appJob.data(), &FlatpakTransactionThread::finished, this, &FlatpakJobTransaction::finishTransaction);
    connect(m_appJob.data(), &FlatpakTransactionThread::progressChanged, this, [this](const QByteArray &currentRef, int progress) {
        if (currentRef == m_app->ref())
            setProgress(progress);
    });
    connect(m_appJob.data(), &FlatpakTransactionThread::speedChanged, this, [this](const QByteArray &currentRef, int speed) {
        if (currentRef == m_app->ref())
            setDownloadSpeed(speed);
    });
    connect(m_appJob.data(), &FlatpakTransactionThread::passiveMessage, this, [this](const QByteArray &currentRef, const QString &message) {
        if (currentRef == m_app->ref())
            passiveMessage(message);
    });

    connect(m_appJob.data(), &FlatpakTransactionThread::webflowStarted, this, [this](const QByteArray &currentRef, const QUrl &url, int id) {
        if (currentRef == m_app->ref())
            webflowStarted(url, id);
    });
    connect(m_appJob.data(), &FlatpakTransactionThread::webflowDone, this, [this](const QByteArray &currentRef, int id) {
        if (currentRef == m_app->ref())
            webflowDone(id);
    });
    QTimer::singleShot(0, this, &FlatpakJobTransaction::start);
}

FlatpakJobTransaction::~FlatpakJobTransaction()
{
    if (m_appJob->isRunning()) {
        cancel();
        m_appJob->wait();
    }
}

void FlatpakJobTransaction::cancel()
{
    m_appJob->cancel();
}

void FlatpakJobTransaction::start()
{
    setStatus(DownloadingStatus);

    s_pending.remove(m_app->installation());
    m_appJob->start();
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
