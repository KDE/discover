/*
 *   SPDX-FileCopyrightText: 2020 Aleix Pol Gonzalez <aleixpol@kde.org>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "UnattendedUpdates.h"
#include "DiscoverNotifier.h"
#include <KIdleTime>
#include <QProcess>
#include <QDateTime>
#include <QDebug>
#include <chrono>

UnattendedUpdates::UnattendedUpdates(DiscoverNotifier* parent)
    : QObject(parent)
{
    connect(parent, &DiscoverNotifier::stateChanged, this, &UnattendedUpdates::checkNewState);
    connect(KIdleTime::instance(), QOverload<int,int>::of(&KIdleTime::timeoutReached), this, &UnattendedUpdates::triggerUpdate);

    checkNewState();
}

UnattendedUpdates::~UnattendedUpdates() noexcept
{
    KIdleTime::instance()->removeAllIdleTimeouts();
}

void UnattendedUpdates::checkNewState()
{
    DiscoverNotifier* notifier = static_cast<DiscoverNotifier*>(parent());
    if (notifier->hasUpdates()) {
        qDebug() << "waiting for an idle moment";
        // If the system is untouched for 1 hour, trigger the unattened update
        using namespace std::chrono_literals;
        KIdleTime::instance()->addIdleTimeout(int(std::chrono::milliseconds(15min).count()));
    } else {
        KIdleTime::instance()->removeAllIdleTimeouts();
    }
}

void UnattendedUpdates::triggerUpdate()
{
    KIdleTime::instance()->removeAllIdleTimeouts();
    DiscoverNotifier* notifier = static_cast<DiscoverNotifier*>(parent());
    if (!notifier->hasUpdates() || notifier->isBusy()) {
        return;
    }

    auto process = new QProcess(this);
    connect(process, &QProcess::errorOccurred, this, [] (QProcess::ProcessError error) {
        qWarning() << "Error running plasma-discover-update" << error;
    });
    connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this, [this, process] (int exitCode, QProcess::ExitStatus exitStatus) {
        qDebug() << "Finished running plasma-discover-update" << exitCode << exitStatus;
        DiscoverNotifier* notifier = static_cast<DiscoverNotifier*>(parent());
        notifier->setBusy(false);
        process->deleteLater();
    });

    notifier->setBusy(true);
    process->start(QStringLiteral("plasma-discover-update"), { QStringLiteral("--offline") });
    qInfo() << "started unattended update" << QDateTime::currentDateTimeUtc();
}
