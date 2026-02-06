/*
 *   SPDX-FileCopyrightText: 2025 Yakup Atahanov
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include <Transaction/Transaction.h>
#include <QMap>
#include <QProcess>

class MCPResource;

/**
 * \class MCPTransaction MCPTransaction.h
 *
 * \brief Handles installation and removal of MCP servers.
 *
 * This transaction class manages the installation and removal of MCP servers
 * using various package managers (npm, pip) or direct binary installation.
 */
class MCPTransaction : public Transaction
{
    Q_OBJECT

public:
    MCPTransaction(MCPResource *resource, Role role);
    ~MCPTransaction() override;

    void cancel() override;
    void proceed() override;

    void setConfiguration(const QMap<QString, QString> &values);

Q_SIGNALS:
    void configRequest(MCPResource *resource);

private Q_SLOTS:
    void onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onProcessError(QProcess::ProcessError error);
    void onProcessReadyReadStandardOutput();
    void onProcessReadyReadStandardError();

private:
    void startInstallation();
    void startRemoval();
    void installNpmPackage();
    void installPipPackage();
    void installBinary();
    void installContainer();
    void removeNpmPackage();
    void removePipPackage();
    void removeBinary();
    void removeContainer();

    void writeManifest();
    void removeManifest();
    void finishTransaction(bool success, const QString &errorMessage = QString());

    MCPResource *m_resource;
    QProcess *m_process = nullptr;
    QString m_outputBuffer;
    QString m_errorBuffer;
    bool m_cancelled = false;
    QMap<QString, QString> m_configValues;
    bool m_waitingForConfig = false;
};
