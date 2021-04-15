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
    , m_newUpdate(false)
    , m_needsReboot(false)
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
        QByteArray readError = process->readAllStandardError();
    });

    // catch data output
    connect(process, &QProcess::readyReadStandardOutput, this, [process, this]() {
        QByteArray readOutput = process->readAllStandardOutput();
        this->getQProcessOutput(readOutput);
    });

    // delete process instance when done, and get the exit status to handle errors.
    QObject::connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), [=](int exitCode, QProcess::ExitStatus exitStatus) {
        qWarning() << "process exited with code " << exitCode;
        recheckSystemUpdate();
        process->deleteLater();
    });

    process->setProcessChannelMode(QProcess::MergedChannels);
    process->start(QStringLiteral("rpm-ostree update --check"));
}

void RpmOstreeNotifier::recheckSystemUpdate()
{
    if (m_newUpdate) {
        Q_EMIT foundUpdates();
    }
}

void RpmOstreeNotifier::getQProcessOutput(QByteArray readOutput)
{
    QList<QByteArray> checkUpdateOutput = readOutput.split('\n');

    for (const QByteArray &output : checkUpdateOutput) {
        if (output.contains(QByteArray("Version"))) {
            m_newUpdate = true;
        }
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
    return m_newUpdate;
}
