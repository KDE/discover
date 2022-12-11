/*
 *   SPDX-FileCopyrightText: 2021 Mariam Fahmy Sobhy <mariamfahmy66@gmail.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "RpmOstreeNotifier.h"

#include <QFile>
#include <QFileSystemWatcher>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QTimer>
#include <QVersionNumber>

RpmOstreeNotifier::RpmOstreeNotifier(QObject *parent)
    : BackendNotifierModule(parent)
    , m_version("")
    , m_hasUpdates(false)
    , m_needsReboot(false)
{
    // Refuse to run on systems not managed by rpm-ostree
    if (!isValid()) {
        qWarning() << "rpm-ostree-notifier: Not starting on a system not managed by rpm-ostree";
        return;
    }

    qInfo() << "rpm-ostree-notifier: Looking for ostree format";
    m_process = new QProcess(this);
    m_stdout = QByteArray();

    // Display stderr
    connect(m_process, &QProcess::readyReadStandardError, [this]() {
        qWarning() << "rpm-ostree (error):" << m_process->readAllStandardError();
    });

    // Store stdout to process as JSON
    connect(m_process, &QProcess::readyReadStandardOutput, [this]() {
        m_stdout += m_process->readAllStandardOutput();
    });

    // Process command result
    connect(m_process, qOverload<int, QProcess::ExitStatus>(&QProcess::finished), [this](int exitCode, QProcess::ExitStatus exitStatus) {
        m_process->deleteLater();
        m_process = nullptr;
        if (exitStatus != QProcess::NormalExit) {
            qWarning() << "rpm-ostree-notifier: Failed to check for existing deployments";
            return;
        }
        if (exitCode != 0) {
            // Unexpected error
            qWarning() << "rpm-ostree-notifier: Failed to check for existing deployments. Exit code:" << exitCode;
            return;
        }

        // Parse stdout as JSON and look at the currently booted deployments to figure out
        // the format used by ostree
        const QJsonDocument jsonDocument = QJsonDocument::fromJson(m_stdout);
        if (!jsonDocument.isObject()) {
            qWarning() << "rpm-ostree-notifier: Could not parse 'rpm-ostree status' output as JSON";
            return;
        }
        const QJsonArray deployments = jsonDocument.object().value("deployments").toArray();
        if (deployments.isEmpty()) {
            qWarning() << "rpm-ostree-notifier: Could not find the deployments in 'rpm-ostree status' JSON output";
            return;
        }
        bool booted;
        for (const QJsonValue &deployment : deployments) {
            booted = deployment.toObject()["booted"].toBool();
            if (!booted) {
                continue;
            }
            // Look for "classic" ostree origin format first
            QString origin = deployment.toObject()["origin"].toString();
            if (!origin.isEmpty()) {
                m_ostreeFormat.reset(new ::OstreeFormat(::OstreeFormat::Format::Classic, origin));
                if (!m_ostreeFormat->isValid()) {
                    // This should never happen
                    qWarning() << "rpm-ostree-notifier: Invalid origin for classic ostree format:" << origin;
                }
            } else {
                // Then look for OCI container format
                origin = deployment.toObject()["container-image-reference"].toString();
                if (!origin.isEmpty()) {
                    m_ostreeFormat.reset(new ::OstreeFormat(::OstreeFormat::Format::OCI, origin));
                    if (!m_ostreeFormat->isValid()) {
                        // This should never happen
                        qWarning() << "rpm-ostree-notifier: Invalid reference for OCI container ostree format:" << origin;
                    }
                } else {
                    // This should never happen
                    m_ostreeFormat.reset(new ::OstreeFormat(::OstreeFormat::Format::Unknown, {}));
                    qWarning() << "rpm-ostree-notifier: Could not find a valid remote ostree format for the booted deployment";
                }
            }
            // Look for the base-version first. This is the case where we have changes layered
            m_version = deployment.toObject()["base-version"].toString();
            if (m_version.isEmpty()) {
                // If empty, look for the regular version (no layered changes)
                m_version = deployment.toObject()["version"].toString();
            }
        }
    });

    m_process->start(QStringLiteral("rpm-ostree"), {QStringLiteral("status"), QStringLiteral("--json")});
    m_process->waitForFinished();
}

bool RpmOstreeNotifier::isValid() const
{
    return QFile::exists(QStringLiteral("/run/ostree-booted"));
}

void RpmOstreeNotifier::recheckSystemUpdateNeeded()
{
    // Refuse to run on systems not managed by rpm-ostree
    if (!isValid()) {
        qWarning() << "rpm-ostree-notifier: Not starting on a system not managed by rpm-ostree";
        return;
    }

    qInfo() << "rpm-ostree-notifier: Checking for system update";
    if (m_ostreeFormat->isClassic()) {
        checkSystemUpdateClassic();
    } else if (m_ostreeFormat->isOCI()) {
        checkSystemUpdateOCI();
    }
}

void RpmOstreeNotifier::checkSystemUpdateClassic()
{
    qInfo() << "rpm-ostree-notifier: Checking for system update (classic format)";

    m_process = new QProcess(this);
    m_stdout = QByteArray();

    // Display stderr
    connect(m_process, &QProcess::readyReadStandardError, [this]() {
        qWarning() << "rpm-ostree (error):" << m_process->readAllStandardError();
    });

    // Display and store stdout
    connect(m_process, &QProcess::readyReadStandardOutput, [this]() {
        QByteArray message = m_process->readAllStandardOutput();
        qInfo() << "rpm-ostree:" << message;
        m_stdout += message;
    });

    // Process command result
    connect(m_process, qOverload<int, QProcess::ExitStatus>(&QProcess::finished), [this](int exitCode, QProcess::ExitStatus exitStatus) {
        m_process->deleteLater();
        m_process = nullptr;
        if (exitStatus != QProcess::NormalExit) {
            qWarning() << "rpm-ostree-notifier: Failed to check for system update";
            return;
        }
        if (exitCode == 77) {
            // rpm-ostree will exit with status 77 when no updates are available
            qInfo() << "rpm-ostree-notifier: No updates available";
            return;
        }
        if (exitCode != 0) {
            qWarning() << "rpm-ostree-notifier: Failed to check for system update. Exit code:" << exitCode;
            return;
        }

        // We have an update available. Let's look if we already have a pending
        // deployment for the new version. First, look for the new version
        // string in rpm-ostree stdout
        QString newVersion, line;
        QString output = QString(m_stdout);
        QTextStream stream(&output);
        while (stream.readLineInto(&line)) {
            if (line.contains(QLatin1String("Version: "))) {
                newVersion = line;
                break;
            }
        }

        // Could not find the new version in rpm-ostree output. This is unlikely
        // to ever happen.
        if (newVersion.isEmpty()) {
            qInfo() << "rpm-ostree-notifier: Could not find the version for the update available";
        }

        // Process the string to get just the version "number".
        newVersion = newVersion.trimmed();
        newVersion.remove(0, QStringLiteral("Version: ").length());
        newVersion.remove(newVersion.size() - QStringLiteral(" (XXXX-XX-XXTXX:XX:XXZ)").length(), newVersion.size() - 1);
        qInfo() << "rpm-ostree-notifier: Found new version:" << newVersion;

        // Have we already notified the user about this update?
        if (newVersion == m_updateVersion) {
            qInfo() << "rpm-ostree-notifier: New version has already been offered. Skipping.";
            return;
        }
        m_updateVersion = newVersion;

        // Look for an existing deployment with this version
        checkForPendingDeployment();
    });

    m_process->start(QStringLiteral("rpm-ostree"), {QStringLiteral("update"), QStringLiteral("--check")});
}

void RpmOstreeNotifier::checkSystemUpdateOCI()
{
    qInfo() << "rpm-ostree-notifier: Checking for system update (OCI format)";

    m_process = new QProcess(this);
    m_stdout = QByteArray();

    // Display stderr
    connect(m_process, &QProcess::readyReadStandardError, [this]() {
        qWarning() << "skopeo (error):" << m_process->readAllStandardError();
    });

    // Store stdout to process as JSON
    connect(m_process, &QProcess::readyReadStandardOutput, [this]() {
        m_stdout += m_process->readAllStandardOutput();
    });

    // Process command result
    connect(m_process, qOverload<int, QProcess::ExitStatus>(&QProcess::finished), [this](int exitCode, QProcess::ExitStatus exitStatus) {
        m_process->deleteLater();
        m_process = nullptr;
        if (exitStatus != QProcess::NormalExit) {
            qWarning() << "rpm-ostree-notifier: Failed to check for updates via skopeo";
            return;
        }
        if (exitCode != 0) {
            // Unexpected error
            qWarning() << "rpm-ostree-notifier: Failed to check for updates via skopeo. Exit code:" << exitCode;
            return;
        }

        // Parse stdout as JSON and look at the container image labels for the version
        const QJsonDocument jsonDocument = QJsonDocument::fromJson(m_stdout);
        if (!jsonDocument.isObject()) {
            qWarning() << "rpm-ostree-notifier: Could not parse 'rpm-ostree status' output as JSON";
            return;
        }

        // Get the version stored in .Labels.version
        const QString newVersion = jsonDocument.object().value("Labels").toObject().value("version").toString();
        if (newVersion.isEmpty()) {
            qInfo() << "rpm-ostree-notifier: Could not get the version from the container labels";
            return;
        }

        QVersionNumber newVersionNumber = QVersionNumber::fromString(newVersion);
        QVersionNumber currentVersionNumber = QVersionNumber::fromString(m_version);
        if (newVersionNumber <= currentVersionNumber) {
            qInfo() << "rpm-ostree-notifier: No new version found";
            return;
        }

        // Have we already notified the user about this update?
        if (newVersion == m_updateVersion) {
            qInfo() << "rpm-ostree-notifier: New version has already been offered. Skipping.";
            return;
        }
        m_updateVersion = newVersion;

        // Look for an existing deployment with this version
        checkForPendingDeployment();
    });

    m_process->start(QStringLiteral("skopeo"), {QStringLiteral("inspect"), "docker://" + m_ostreeFormat->repo() + ":" + m_ostreeFormat->tag()});
}

void RpmOstreeNotifier::checkForPendingDeployment()
{
    qInfo() << "rpm-ostree-notifier: Looking at existing deployments";
    m_process = new QProcess(this);
    m_stdout = QByteArray();

    // Display stderr
    connect(m_process, &QProcess::readyReadStandardError, [this]() {
        QByteArray message = m_process->readAllStandardError();
        qWarning() << "rpm-ostree (error):" << message;
    });

    // Store stdout to process as JSON
    connect(m_process, &QProcess::readyReadStandardOutput, [this]() {
        QByteArray message = m_process->readAllStandardOutput();
        m_stdout += message;
    });

    // Process command result
    connect(m_process, qOverload<int, QProcess::ExitStatus>(&QProcess::finished), [this](int exitCode, QProcess::ExitStatus exitStatus) {
        m_process->deleteLater();
        m_process = nullptr;
        if (exitStatus != QProcess::NormalExit) {
            qWarning() << "rpm-ostree-notifier: Failed to check for existing deployments";
            return;
        }
        if (exitCode != 0) {
            // Unexpected error
            qWarning() << "rpm-ostree-notifier: Failed to check for existing deployments. Exit code:" << exitCode;
            return;
        }

        // Parse stdout as JSON and look at the deployments for a pending
        // deployment for the new version.
        const QJsonDocument jsonDocument = QJsonDocument::fromJson(m_stdout);
        if (!jsonDocument.isObject()) {
            qWarning() << "rpm-ostree-notifier: Could not parse 'rpm-ostree status' output as JSON";
            return;
        }
        const QJsonArray deployments = jsonDocument.object().value("deployments").toArray();
        if (deployments.isEmpty()) {
            qWarning() << "rpm-ostree-notifier: Could not find the deployments in 'rpm-ostree status' JSON output";
            return;
        }
        QString version;
        for (const QJsonValue &deployment : deployments) {
            version = deployment.toObject()["base-version"].toString();
            if (version.isEmpty()) {
                version = deployment.toObject()["version"].toString();
            }
            if (version.isEmpty()) {
                qInfo() << "rpm-ostree-notifier: Could not read version for deployment:" << deployment;
                continue;
            }
            if (version == m_updateVersion) {
                qInfo() << "rpm-ostree-notifier: Found an existing deployment for the update available";
                if (!m_needsReboot) {
                    m_needsReboot = true;
                    Q_EMIT needsRebootChanged();
                    return;
                }
            }
        }

        // No deployment found for the new version. Let's notify the user.
        qInfo() << "rpm-ostree-notifier: Notifying that a new update is available";
        m_hasUpdates = true;
        Q_EMIT foundUpdates();

        // TODO: Look for security updates fixed by this new deployment
    });

    m_process->start(QStringLiteral("rpm-ostree"), {QStringLiteral("status"), QStringLiteral("--json")});
}

bool RpmOstreeNotifier::hasSecurityUpdates()
{
    return false;
}

bool RpmOstreeNotifier::needsReboot() const
{
    return m_needsReboot;
}

bool RpmOstreeNotifier::hasUpdates()
{
    return m_hasUpdates;
}
