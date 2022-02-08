/*
 *   SPDX-FileCopyrightText: 2020 Aleix Pol Gonzalez <aleixpol@kde.org>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "UnattendedUpdates.h"
#include "DiscoverNotifier.h"
#include "updatessettings.h"
#include <KIdleTime>
#include <QDateTime>
#include <QDebug>
#include <QProcess>
#include <chrono>

UnattendedUpdates::UnattendedUpdates(DiscoverNotifier *parent)
    : QObject(parent)
{
    connect(parent, &DiscoverNotifier::stateChanged, this, &UnattendedUpdates::checkNewState);
    connect(KIdleTime::instance(), QOverload<int, int>::of(&KIdleTime::timeoutReached), this, &UnattendedUpdates::triggerUpdate);

    checkNewState();
}

UnattendedUpdates::~UnattendedUpdates() noexcept
{
    if (m_idleTimeoutId.has_value()) {
        KIdleTime::instance()->removeIdleTimeout(m_idleTimeoutId.value());
    }
}

void UnattendedUpdates::checkNewState()
{
    using namespace std::chrono_literals;
    DiscoverNotifier *notifier = static_cast<DiscoverNotifier *>(parent());

    // Only allow offline updating every 3h. It should keep some peace to our users, especially on rolling distros
    const QDateTime updateableTime = notifier->settings()->lastUnattendedTrigger().addSecs(std::chrono::seconds(3h).count());
    if (updateableTime > QDateTime::currentDateTimeUtc()) {
        qDebug() << "skipping update, already updated on" << notifier->settings()->lastUnattendedTrigger().toString();
        return;
    }

    const bool hasUpdates = notifier->hasUpdates();
    if (hasUpdates && !m_idleTimeoutId.has_value()) {
        qDebug() << "waiting for an idle moment";
        // If the system is untouched for 15 minutes, trigger the unattened update
        m_idleTimeoutId = KIdleTime::instance()->addIdleTimeout(int(std::chrono::milliseconds(15min).count()));
    } else if (!hasUpdates && m_idleTimeoutId.has_value()) {
        qDebug() << "nothing to do";
        KIdleTime::instance()->removeIdleTimeout(m_idleTimeoutId.value());
        m_idleTimeoutId.reset();
    }
}

void UnattendedUpdates::triggerUpdate(int timeoutId)
{
    if (!m_idleTimeoutId.has_value() || timeoutId != m_idleTimeoutId.value()) {
        return;
    }

    KIdleTime::instance()->removeIdleTimeout(m_idleTimeoutId.value());
    m_idleTimeoutId.reset();

    DiscoverNotifier *notifier = static_cast<DiscoverNotifier *>(parent());
    if (!notifier->hasUpdates() || notifier->isBusy()) {
        return;
    }

    auto process = new QProcess(this);
    connect(process, &QProcess::errorOccurred, this, [](QProcess::ProcessError error) {
        qWarning() << "Error running plasma-discover-update" << error;
    });
    connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this, [this, process](int exitCode, QProcess::ExitStatus exitStatus) {
        qDebug() << "Finished running plasma-discover-update" << exitCode << exitStatus;
        DiscoverNotifier *notifier = static_cast<DiscoverNotifier *>(parent());
        notifier->setBusy(false);
        process->deleteLater();
        notifier->settings()->setLastUnattendedTrigger(QDateTime::currentDateTimeUtc());
    });

    notifier->setBusy(true);
    process->start(QStringLiteral("plasma-discover-update"), {QStringLiteral("--offline")});
    qInfo() << "started unattended update" << QDateTime::currentDateTimeUtc();
}
