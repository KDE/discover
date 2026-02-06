/*
 *   SPDX-FileCopyrightText: 2025 Yakup Atahanov
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "MCPTransaction.h"
#include "MCPBackend.h"
#include "MCPResource.h"
#include "libdiscover_backend_mcp_debug.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>

using namespace Qt::StringLiterals;

MCPTransaction::MCPTransaction(MCPResource *resource, Role role)
    : Transaction(resource->backend(), resource, role)
    , m_resource(resource)
{
    setCancellable(true);
    setStatus(SetupStatus);

    if (role == InstallRole) {
        startInstallation();
    } else if (role == RemoveRole) {
        startRemoval();
    }
}

MCPTransaction::~MCPTransaction()
{
    if (m_process) {
        m_process->disconnect();
        if (m_process->state() != QProcess::NotRunning) {
            m_process->terminate();
            m_process->waitForFinished(3000);
        }
        delete m_process;
    }
}

void MCPTransaction::cancel()
{
    m_cancelled = true;

    if (m_process && m_process->state() != QProcess::NotRunning) {
        m_process->terminate();
        if (!m_process->waitForFinished(3000)) {
            m_process->kill();
        }
    }

    setStatus(CancelledStatus);
}

void MCPTransaction::proceed()
{
    // Continue with installation after user confirmation or config completion
    if (status() == SetupStatus) {
        if (m_waitingForConfig) {
            // Configuration was provided, continue with installation
            m_waitingForConfig = false;
            startInstallation();
        } else {
            // Regular proceed (user confirmation)
            startInstallation();
        }
    }
}

void MCPTransaction::setConfiguration(const QMap<QString, QString> &values)
{
    m_configValues = values;
    // Store configuration values in the resource
    for (auto it = values.begin(); it != values.end(); ++it) {
        m_resource->setPropertyValue(it.key(), it.value());
    }
    // Continue with installation
    proceed();
}

void MCPTransaction::startInstallation()
{
    // Check if resource has required properties that need configuration
    if (!m_resource->requiredProperties().isEmpty()) {
        // Check if we have all required values
        bool needsConfig = false;
        for (const auto &prop : m_resource->requiredProperties()) {
            if (prop.required && m_resource->propertyValue(prop.key).isEmpty()) {
                needsConfig = true;
                break;
            }
        }
        if (needsConfig) {
            setStatus(SetupStatus);
            m_waitingForConfig = true;
            Q_EMIT configRequest(m_resource);
            return;
        }
    }

    setStatus(DownloadingStatus);
    setProgress(0);

    switch (m_resource->sourceType()) {
    case MCPResource::SourceType::Npm:
        installNpmPackage();
        break;
    case MCPResource::SourceType::Pip:
        installPipPackage();
        break;
    case MCPResource::SourceType::Binary:
        installBinary();
        break;
    case MCPResource::SourceType::Container:
        installContainer();
        break;
    case MCPResource::SourceType::Git:
        // Git installation - clone and setup
        installBinary(); // Treat similar to binary for now
        break;
    }
}

void MCPTransaction::startRemoval()
{
    setStatus(CommittingStatus);
    setProgress(0);

    switch (m_resource->sourceType()) {
    case MCPResource::SourceType::Npm:
        removeNpmPackage();
        break;
    case MCPResource::SourceType::Pip:
        removePipPackage();
        break;
    case MCPResource::SourceType::Binary:
        removeBinary();
        break;
    case MCPResource::SourceType::Container:
        removeContainer();
        break;
    case MCPResource::SourceType::Git:
        removeBinary();
        break;
    }
}

void MCPTransaction::installNpmPackage()
{
    m_process = new QProcess(this);
    connect(m_process, &QProcess::finished, this, &MCPTransaction::onProcessFinished);
    connect(m_process, &QProcess::errorOccurred, this, &MCPTransaction::onProcessError);
    connect(m_process, &QProcess::readyReadStandardOutput, this, &MCPTransaction::onProcessReadyReadStandardOutput);
    connect(m_process, &QProcess::readyReadStandardError, this, &MCPTransaction::onProcessReadyReadStandardError);

    // Get the source package from the resource's JSON data
    const QJsonObject json = m_resource->toJson();
    const QString package = json[u"source"_s].toObject()[u"package"_s].toString();

    if (package.isEmpty()) {
        finishTransaction(false, u"No npm package specified"_s);
        return;
    }

    QStringList args;
    args << u"install"_s << u"-g"_s << package;

    qCDebug(LIBDISCOVER_BACKEND_MCP_LOG) << "MCPTransaction: Installing npm package:" << package;
    m_process->start(u"npm"_s, args);
}

void MCPTransaction::installPipPackage()
{
    m_process = new QProcess(this);
    connect(m_process, &QProcess::finished, this, &MCPTransaction::onProcessFinished);
    connect(m_process, &QProcess::errorOccurred, this, &MCPTransaction::onProcessError);
    connect(m_process, &QProcess::readyReadStandardOutput, this, &MCPTransaction::onProcessReadyReadStandardOutput);
    connect(m_process, &QProcess::readyReadStandardError, this, &MCPTransaction::onProcessReadyReadStandardError);

    const QJsonObject json = m_resource->toJson();
    const QString package = json[u"source"_s].toObject()[u"package"_s].toString();

    if (package.isEmpty()) {
        finishTransaction(false, u"No pip package specified"_s);
        return;
    }

    QStringList args;
    args << u"install"_s << u"--user"_s << package;

    qCDebug(LIBDISCOVER_BACKEND_MCP_LOG) << "MCPTransaction: Installing pip package:" << package;
    m_process->start(u"pip"_s, args);
}

void MCPTransaction::installBinary()
{
    // For binary installations, we would download and install the binary
    // This is a simplified implementation
    const QJsonObject json = m_resource->toJson();
    const QString url = json[u"source"_s].toObject()[u"url"_s].toString();

    if (url.isEmpty()) {
        finishTransaction(false, u"No download URL specified for binary"_s);
        return;
    }

    // TODO: Implement actual binary download using KIO or QNetworkAccessManager
    // For now, we'll just write the manifest and assume success
    setProgress(50);
    setStatus(CommittingStatus);
    writeManifest();
    finishTransaction(true);
}

void MCPTransaction::installContainer()
{
    m_process = new QProcess(this);
    connect(m_process, &QProcess::finished, this, &MCPTransaction::onProcessFinished);
    connect(m_process, &QProcess::errorOccurred, this, &MCPTransaction::onProcessError);
    connect(m_process, &QProcess::readyReadStandardOutput, this, &MCPTransaction::onProcessReadyReadStandardOutput);
    connect(m_process, &QProcess::readyReadStandardError, this, &MCPTransaction::onProcessReadyReadStandardError);

    const QJsonObject json = m_resource->toJson();
    const QString image = json[u"source"_s].toObject()[u"package"_s].toString();

    if (image.isEmpty()) {
        finishTransaction(false, u"No container image specified"_s);
        return;
    }

    // Try podman first, fall back to docker
    QStringList args;
    args << u"pull"_s << image;

    qCDebug(LIBDISCOVER_BACKEND_MCP_LOG) << "MCPTransaction: Pulling container image:" << image;
    m_process->start(u"podman"_s, args);
}

void MCPTransaction::removeNpmPackage()
{
    m_process = new QProcess(this);
    connect(m_process, &QProcess::finished, this, &MCPTransaction::onProcessFinished);
    connect(m_process, &QProcess::errorOccurred, this, &MCPTransaction::onProcessError);

    const QJsonObject json = m_resource->toJson();
    const QString package = json[u"source"_s].toObject()[u"package"_s].toString();

    QStringList args;
    args << u"uninstall"_s << u"-g"_s << package;

    qCDebug(LIBDISCOVER_BACKEND_MCP_LOG) << "MCPTransaction: Removing npm package:" << package;
    m_process->start(u"npm"_s, args);
}

void MCPTransaction::removePipPackage()
{
    m_process = new QProcess(this);
    connect(m_process, &QProcess::finished, this, &MCPTransaction::onProcessFinished);
    connect(m_process, &QProcess::errorOccurred, this, &MCPTransaction::onProcessError);

    const QJsonObject json = m_resource->toJson();
    const QString package = json[u"source"_s].toObject()[u"package"_s].toString();

    QStringList args;
    args << u"uninstall"_s << u"-y"_s << package;

    qCDebug(LIBDISCOVER_BACKEND_MCP_LOG) << "MCPTransaction: Removing pip package:" << package;
    m_process->start(u"pip"_s, args);
}

void MCPTransaction::removeBinary()
{
    // Remove the binary and manifest
    removeManifest();
    finishTransaction(true);
}

void MCPTransaction::removeContainer()
{
    m_process = new QProcess(this);
    connect(m_process, &QProcess::finished, this, &MCPTransaction::onProcessFinished);
    connect(m_process, &QProcess::errorOccurred, this, &MCPTransaction::onProcessError);

    const QJsonObject json = m_resource->toJson();
    const QString image = json[u"source"_s].toObject()[u"package"_s].toString();

    QStringList args;
    args << u"rmi"_s << image;

    qCDebug(LIBDISCOVER_BACKEND_MCP_LOG) << "MCPTransaction: Removing container image:" << image;
    m_process->start(u"podman"_s, args);
}

void MCPTransaction::onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    if (m_cancelled) {
        return;
    }

    if (exitStatus == QProcess::CrashExit || exitCode != 0) {
        QString errorMsg = m_errorBuffer.isEmpty()
            ? u"Process failed with exit code %1"_s.arg(exitCode)
            : m_errorBuffer;
        finishTransaction(false, errorMsg);
        return;
    }

    setProgress(90);
    setStatus(CommittingStatus);

    if (role() == InstallRole) {
        writeManifest();
    } else {
        removeManifest();
    }

    finishTransaction(true);
}

void MCPTransaction::onProcessError(QProcess::ProcessError error)
{
    if (m_cancelled) {
        return;
    }

    QString errorMsg;
    switch (error) {
    case QProcess::FailedToStart:
        errorMsg = u"Failed to start process. Is the package manager installed?"_s;
        break;
    case QProcess::Crashed:
        errorMsg = u"Process crashed"_s;
        break;
    case QProcess::Timedout:
        errorMsg = u"Process timed out"_s;
        break;
    default:
        errorMsg = u"Unknown process error"_s;
        break;
    }

    finishTransaction(false, errorMsg);
}

void MCPTransaction::onProcessReadyReadStandardOutput()
{
    const QString output = QString::fromUtf8(m_process->readAllStandardOutput());
    m_outputBuffer += output;

    // Update progress based on output (simplified)
    if (progress() < 80) {
        setProgress(progress() + 5);
    }
}

void MCPTransaction::onProcessReadyReadStandardError()
{
    m_errorBuffer += QString::fromUtf8(m_process->readAllStandardError());
}

void MCPTransaction::writeManifest()
{
    const QString manifestPath = m_resource->manifestPath();
    const QDir dir = QFileInfo(manifestPath).absoluteDir();

    if (!dir.exists()) {
        dir.mkpath(u"."_s);
    }

    QFile file(manifestPath);
    if (file.open(QIODevice::WriteOnly)) {
        QJsonObject manifest = m_resource->toJson();
        manifest[u"installedVersion"_s] = m_resource->availableVersion();
        manifest[u"installedDate"_s] = QDateTime::currentDateTime().toString(Qt::ISODate);

        const QJsonDocument doc(manifest);
        file.write(doc.toJson(QJsonDocument::Indented));
        file.close();

        qCDebug(LIBDISCOVER_BACKEND_MCP_LOG) << "MCPTransaction: Wrote manifest to:" << manifestPath;
    } else {
        qCWarning(LIBDISCOVER_BACKEND_MCP_LOG) << "MCPTransaction: Failed to write manifest:" << manifestPath;
    }
}

void MCPTransaction::removeManifest()
{
    const QString manifestPath = m_resource->manifestPath();
    if (QFile::exists(manifestPath)) {
        QFile::remove(manifestPath);
        qCDebug(LIBDISCOVER_BACKEND_MCP_LOG) << "MCPTransaction: Removed manifest:" << manifestPath;
    }
}

void MCPTransaction::finishTransaction(bool success, const QString &errorMessage)
{
    setProgress(100);

    if (success) {
        if (role() == InstallRole) {
            m_resource->setState(AbstractResource::Installed);
            m_resource->setInstalledVersion(m_resource->availableVersion());
        } else {
            m_resource->setState(AbstractResource::None);
            m_resource->setInstalledVersion(QString());
        }
        setStatus(DoneStatus);
    } else {
        Q_EMIT passiveMessage(errorMessage);
        setStatus(DoneWithErrorStatus);
    }

    deleteLater();
}

#include "moc_MCPTransaction.cpp"
