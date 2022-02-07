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
    KIdleTime::instance()->removeAllIdleTimeouts();
}

void UnattendedUpdates::checkNewState()
{
    using namespace std::chrono_literals;
    DiscoverNotifier *notifier = static_cast<DiscoverNotifier *>(parent());

    // Only allow offline updating every 3h. It should keep some peace to our users, especially on rolling distros
    const QDateTime updateableTime = notifier->settings()->lastUnattendedTrigger().addSecs((3h).count());
    if (updateableTime > QDateTime::currentDateTimeUtc()) {
        qDebug() << "skipping update, already updated on" << notifier->settings()->lastUnattendedTrigger().toString();
        return;
    }

    if (!KIdleTime::instance()->idleTimeouts().isEmpty()) {
        qDebug() << "already waiting for an idle time";
        return;
    }

    if (notifier->hasUpdates()) {
        qDebug() << "waiting for an idle moment";
        // If the system is untouched for 15 minutes, trigger the unattened update
        KIdleTime::instance()->addIdleTimeout(int(std::chrono::milliseconds(15min).count()));
    } else {
        qDebug() << "nothing to do";
        KIdleTime::instance()->removeAllIdleTimeouts();
    }
}

void UnattendedUpdates::triggerUpdate()
{
    KIdleTime::instance()->removeAllIdleTimeouts();
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
