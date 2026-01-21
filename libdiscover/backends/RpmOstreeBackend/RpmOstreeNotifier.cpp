/*
 *   SPDX-FileCopyrightText: 2021 Mariam Fahmy Sobhy <mariamfahmy66@gmail.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "RpmOstreeNotifier.h"

#include <QDirIterator>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QTimer>
#include <QVersionNumber>

#include "libdiscover_rpm-ostree_debug.h"

RpmOstreeNotifier::RpmOstreeNotifier(QObject *parent)
    : BackendNotifierModule(parent)
    , m_version(QString())
    , m_hasUpdates(false)
    , m_needsReboot(false)
{
    // Refuse to run on systems not managed by rpm-ostree
    if (!isValid()) {
        qCWarning(RPMOSTREE_LOG) << "Not starting on a system not managed by rpm-ostree";
        return;
    }

    // Setup a  watcher to trigger a check for reboot when the deployments are changed
    // and there is thus likely an new deployment installed following an update.
    m_watcher = new QFileSystemWatcher(this);

    // We also setup a timer to avoid triggering a check immediately when a new
    // deployment is made available and instead wait a bit to let things settle down.
    m_timer = new QTimer(this);
    m_timer->setSingleShot(true);
    // Wait 10 seconds for all rpm-ostree operations to complete
    m_timer->setInterval(10000);
    connect(m_timer, &QTimer::timeout, this, &RpmOstreeNotifier::checkForPendingDeployment);

    // Find all ostree managed system installations available. There is usually only one but
    // doing that dynamically here avoids hardcoding a specific value or doing a DBus call.
    QDirIterator it(QStringLiteral("/ostree/deploy/"), QDir::AllDirs | QDir::NoDotAndDotDot);
    while (it.hasNext()) {
        QString path = QStringLiteral("%1/deploy/").arg(it.next());
        m_watcher->addPath(path);
        qCInfo(RPMOSTREE_LOG) << "Looking for new deployments in" << path;
    }
    connect(m_watcher, &QFileSystemWatcher::directoryChanged, [this]() {
        m_timer->start();
    });

    qCInfo(RPMOSTREE_LOG) << "Looking for ostree format";

    QProcess process;

    // Display stderr
    connect(&process, &QProcess::readyReadStandardError, this, [&process] {
        qCWarning(RPMOSTREE_LOG) << "rpm-ostree (error):" << process.readAllStandardError();
    });

    process.start(QStringLiteral("rpm-ostree"), {QStringLiteral("status"), QStringLiteral("--json")});
    process.waitForFinished();

    // Process command result
    if (process.exitStatus() != QProcess::NormalExit) {
        qCWarning(RPMOSTREE_LOG) << "Failed to check for existing deployments";
        return;
    }
    if (process.exitCode() != 0) {
        // Unexpected error
        qCWarning(RPMOSTREE_LOG) << "Failed to check for existing deployments. Exit code:" << process.exitCode();
        return;
    }

        // Parse stdout as JSON and look at the currently booted deployments to figure out
        // the format used by ostree
    const QJsonDocument jsonDocument = QJsonDocument::fromJson(process.readAllStandardOutput());
    if (!jsonDocument.isObject()) {
        qCWarning(RPMOSTREE_LOG) << "Could not parse 'rpm-ostree status' output as JSON";
        return;
    }
        const QJsonArray deployments = jsonDocument.object().value(QLatin1String("deployments")).toArray();
        if (deployments.isEmpty()) {
            qCWarning(RPMOSTREE_LOG) << "Could not find the deployments in 'rpm-ostree status' JSON output";
            return;
        }
        bool booted;
        for (const QJsonValue &deployment : deployments) {
            booted = deployment.toObject()[QLatin1String("booted")].toBool();
            if (!booted) {
                continue;
            }
            // Look for "classic" ostree origin format first
            QString origin = deployment.toObject()[QLatin1String("origin")].toString();
            if (!origin.isEmpty()) {
                m_ostreeFormat.reset(new ::OstreeFormat(::OstreeFormat::Format::Classic, origin));
                if (!m_ostreeFormat->isValid()) {
                    // This should never happen
                    qCWarning(RPMOSTREE_LOG) << "Invalid origin for classic ostree format:" << origin;
                }
            } else {
                // Then look for OCI container format
                origin = deployment.toObject()[QLatin1String("container-image-reference")].toString();
                if (!origin.isEmpty()) {
                    m_ostreeFormat.reset(new ::OstreeFormat(::OstreeFormat::Format::OCI, origin));
                    if (!m_ostreeFormat->isValid()) {
                        // This should never happen
                        qCWarning(RPMOSTREE_LOG) << "Invalid reference for OCI container ostree format:" << origin;
                    }
                } else {
                    // This should never happen
                    m_ostreeFormat.reset(new ::OstreeFormat(::OstreeFormat::Format::Unknown, {}));
                    qCWarning(RPMOSTREE_LOG) << "Could not find a valid remote ostree format for the booted deployment";
                }
            }
            // Look for the base-version first. This is the case where we have changes layered
            m_version = deployment.toObject()[QLatin1String("base-version")].toString();
            if (m_version.isEmpty()) {
                // If empty, look for the regular version (no layered changes)
                m_version = deployment.toObject()[QLatin1String("version")].toString();
            }
        }
}

bool RpmOstreeNotifier::isValid() const
{
    return QFile::exists(QStringLiteral("/run/ostree-booted"));
}

void RpmOstreeNotifier::recheckSystemUpdateNeeded()
{
    // Refuse to run on systems not managed by rpm-ostree
    if (!isValid()) {
        qCWarning(RPMOSTREE_LOG) << "Not starting on a system not managed by rpm-ostree";
        return;
    }

    qCInfo(RPMOSTREE_LOG) << "Checking for system update";
    if (m_ostreeFormat->isClassic()) {
        checkSystemUpdateClassic();
    } else if (m_ostreeFormat->isOCI()) {
        checkSystemUpdateOCI();
    }
}

void RpmOstreeNotifier::checkSystemUpdateClassic()
{
    qCInfo(RPMOSTREE_LOG) << "Checking for system update (classic format)";

    auto process = new QProcess;

    // Display stderr
    connect(process, &QProcess::readyReadStandardError, this, [this, process]() {
        qCWarning(RPMOSTREE_LOG) << "rpm-ostree (error):" << process->readAllStandardError();
    });

    // Process command result
    connect(process, &QProcess::finished, this, [this, process](int exitCode, QProcess::ExitStatus exitStatus) {
        process->deleteLater();
        if (exitStatus != QProcess::NormalExit) {
            qCWarning(RPMOSTREE_LOG) << "Failed to check for system update";
            return;
        }
        if (exitCode == 77) {
            // rpm-ostree will exit with status 77 when no updates are available
            qCInfo(RPMOSTREE_LOG) << "No updates available";
            return;
        }
        if (exitCode != 0) {
            qCWarning(RPMOSTREE_LOG) << "Failed to check for system update. Exit code:" << exitCode;
            return;
        }

        // We have an update available. Let's look if we already have a pending
        // deployment for the new version. First, look for the new version
        // string in rpm-ostree stdout
        QString newVersion, line;
        QString output = QString::fromUtf8(process->readAllStandardOutput());
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
            qCInfo(RPMOSTREE_LOG) << "Could not find the version for the update available";
            return;
        }

        // Process the string to get just the version "number".
        newVersion = newVersion.trimmed();
        newVersion.remove(0, QStringLiteral("Version: ").length());
        newVersion.remove(newVersion.size() - QStringLiteral(" (XXXX-XX-XXTXX:XX:XXZ)").length(), newVersion.size() - 1);
        qCInfo(RPMOSTREE_LOG) << "Found new version:" << newVersion;

        // Have we already notified the user about this update?
        if (newVersion == m_updateVersion) {
            qCInfo(RPMOSTREE_LOG) << "New version has already been offered. Skipping.";
            return;
        }
        m_updateVersion = newVersion;

        // Look for an existing deployment with this version
        checkForPendingDeployment();
    });

    process->start(QStringLiteral("rpm-ostree"), {QStringLiteral("update"), QStringLiteral("--check")});
}

void RpmOstreeNotifier::checkSystemUpdateOCI()
{
    qCInfo(RPMOSTREE_LOG) << "Checking for system update (OCI format)";

    if (m_ostreeFormat->isLocalOCI()) {
        qCInfo(RPMOSTREE_LOG) << "Ignoring update checks for local OCI transports";
        return;
    }

    auto process = new QProcess;

    // Display stderr
    connect(process, &QProcess::readyReadStandardError, this, [this, process]() {
        qCWarning(RPMOSTREE_LOG) << "skopeo (error):" << process->readAllStandardError();
    });

    // Process command result
    connect(process, &QProcess::finished, this, [this, process](int exitCode, QProcess::ExitStatus exitStatus) {
        if (exitStatus != QProcess::NormalExit) {
            qCWarning(RPMOSTREE_LOG) << "Failed to check for updates via skopeo";
            return;
        }
        if (exitCode != 0) {
            // Unexpected error
            qCWarning(RPMOSTREE_LOG) << "Failed to check for updates via skopeo. Exit code:" << exitCode;
            return;
        }

        // Parse stdout as JSON and look at the container image labels for the version
        const QJsonDocument jsonDocument = QJsonDocument::fromJson(process->readAllStandardOutput());
        if (!jsonDocument.isObject()) {
            qCWarning(RPMOSTREE_LOG) << "Could not parse 'rpm-ostree status' output as JSON";
            return;
        }

        // Get the version stored in 'org.opencontainers.image.version' label
        QString newVersion =
            jsonDocument.object().value(QLatin1String("Labels")).toObject().value(QLatin1String("org.opencontainers.image.version")).toString();
        if (newVersion.isEmpty()) {
            // Check the 'version' label for compatibility with older rpm-ostree releases (< v2024.03)
            newVersion = jsonDocument.object().value(QLatin1String("Labels")).toObject().value(QLatin1String("version")).toString();
            if (newVersion.isEmpty()) {
                qCWarning(RPMOSTREE_LOG) << "Could not get the version from the container labels";
                return;
            }
        }

        QVersionNumber newVersionNumber = QVersionNumber::fromString(newVersion);
        QVersionNumber currentVersionNumber = QVersionNumber::fromString(m_version);
        if (newVersionNumber <= currentVersionNumber) {
            qCInfo(RPMOSTREE_LOG) << "No new version found";
            return;
        }

        // Have we already notified the user about this update?
        if (newVersion == m_updateVersion) {
            qCInfo(RPMOSTREE_LOG) << "New version has already been offered. Skipping.";
            return;
        }
        m_updateVersion = newVersion;

        // Look for an existing deployment with this version
        checkForPendingDeployment();
    });

    process->start(QStringLiteral("skopeo"),
                   {QStringLiteral("inspect"),
                    QStringLiteral("--no-tags"),
                    QStringLiteral("docker://") + m_ostreeFormat->repo() + QStringLiteral(":") + m_ostreeFormat->tag()});
}

void RpmOstreeNotifier::checkForPendingDeployment()
{
    qCInfo(RPMOSTREE_LOG) << "Looking at existing deployments";
    auto process = new QProcess;

    // Display stderr
    connect(process, &QProcess::readyReadStandardError, this, [this, process]() {
        QByteArray message = process->readAllStandardError();
        qCWarning(RPMOSTREE_LOG) << "rpm-ostree (error):" << message;
    });

    // Process command result
    connect(process, &QProcess::finished, this, [this, process](int exitCode, QProcess::ExitStatus exitStatus) {
        process->deleteLater();
        if (exitStatus != QProcess::NormalExit) {
            qCWarning(RPMOSTREE_LOG) << "Failed to check for existing deployments";
            return;
        }
        if (exitCode != 0) {
            // Unexpected error
            qCWarning(RPMOSTREE_LOG) << "Failed to check for existing deployments. Exit code:" << exitCode;
            return;
        }

        // Parse stdout as JSON and look at the deployments for a pending
        // deployment for the new version.
        const QJsonDocument jsonDocument = QJsonDocument::fromJson(process->readAllStandardOutput());
        if (!jsonDocument.isObject()) {
            qCWarning(RPMOSTREE_LOG) << "Could not parse 'rpm-ostree status' output as JSON";
            return;
        }
        const QJsonArray deployments = jsonDocument.object().value(QLatin1String("deployments")).toArray();
        if (deployments.isEmpty()) {
            qCWarning(RPMOSTREE_LOG) << "Could not find the deployments in 'rpm-ostree status' JSON output";
            return;
        }
        QString version;
        for (const QJsonValue &deployment : deployments) {
            version = deployment.toObject()[QLatin1String("base-version")].toString();
            if (version.isEmpty()) {
                version = deployment.toObject()[QLatin1String("version")].toString();
            }
            if (version.isEmpty()) {
                qCInfo(RPMOSTREE_LOG) << "Could not read version for deployment:" << deployment;
                continue;
            }
            if (version == m_updateVersion) {
                qCInfo(RPMOSTREE_LOG) << "Found an existing deployment for the update available";
                if (!m_needsReboot) {
                    qCInfo(RPMOSTREE_LOG) << "Notifying that a reboot is needed";
                    m_needsReboot = true;
                    Q_EMIT needsRebootChanged();
                }
                return;
            }
        }

        // Reaching here means that no deployment has been found for the new version.
        qCInfo(RPMOSTREE_LOG) << "Notifying that a new update is available";
        m_hasUpdates = true;
        Q_EMIT foundUpdates();

        // TODO: Look for security updates fixed by this new deployment
    });

    process->start(QStringLiteral("rpm-ostree"), {QStringLiteral("status"), QStringLiteral("--json")});
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

#include "moc_RpmOstreeNotifier.cpp"
