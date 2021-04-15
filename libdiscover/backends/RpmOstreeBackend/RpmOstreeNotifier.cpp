/*
 *   SPDX-FileCopyrightText: 2021 Mariam Fahmy Sobhy <mariamfahmy66@gmail.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "RpmOstreeNotifier.h"
#include <QDebug>
#include <QFileSystemWatcher>
#include <QProcess>
#include <QTimer>

RpmOstreeNotifier::RpmOstreeNotifier(QObject *parent)
    : BackendNotifierModule(parent)
{
    QFileSystemWatcher *watcher = new QFileSystemWatcher(this);
    watcher->addPath(QStringLiteral("/ostree/deploy/fedora/deploy/"));
    connect(watcher, &QFileSystemWatcher::fileChanged, this, &RpmOstreeNotifier::recheckSystemUpdateNeeded);

    QFileSystemWatcher *fileWatcher = new QFileSystemWatcher(this);
    fileWatcher->addPath(QStringLiteral("/tmp/discover-ostree-changed"));
    connect(fileWatcher, &QFileSystemWatcher::fileChanged, this, &RpmOstreeNotifier::nowNeedsReboot);
}

RpmOstreeNotifier::~RpmOstreeNotifier()
{
}

void RpmOstreeNotifier::recheckSystemUpdateNeeded()
{
    QProcess *process = new QProcess();

    connect(process, &QProcess::readyReadStandardError, [process]() {
        qDebug() << "rpm-ostree errors" << process->readAllStandardError().constData();
    });

    // handle the output if successful
    QObject::connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), [=](int exitCode, QProcess::ExitStatus exitStatus) {
        qWarning() << "process exited with code " << exitCode << exitStatus;
        if (exitCode == 0) {
            readUpdateOutput(process);
        }
        process->deleteLater();
    });

    process->setProcessChannelMode(QProcess::MergedChannels);
    process->start(QStringLiteral("rpm-ostree"), {QStringLiteral("update"), QStringLiteral("--check")});
}

void RpmOstreeNotifier::readUpdateOutput(QIODevice *device)
{
    const bool hadUpdates = m_hasUpdates;
    m_hasUpdates = false;
    QTextStream stream(device);
    for (QString line = stream.readLine(); stream.readLineInto(&line);) {
        m_hasUpdates |= line.contains(QLatin1String("Version"));
    }

    if (m_hasUpdates != hadUpdates) {
        Q_EMIT foundUpdates();
    }
}

void RpmOstreeNotifier::nowNeedsReboot()
{
    if (!m_needsReboot) {
        m_needsReboot = true;
        Q_EMIT needsRebootChanged();
    }
}
bool RpmOstreeNotifier::needsReboot() const
{
    return m_needsReboot;
}

bool RpmOstreeNotifier::hasUpdates()
{
    return m_hasUpdates;
}
