/*
 *   SPDX-FileCopyrightText: 2021 Mariam Fahmy Sobhy <mariamfahmy66@gmail.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "RpmOstreeTransaction.h"

#include <KLocalizedString>

#include <QDebug>

static const QString TransactionConnection = QStringLiteral("discover_transaction");
static const QString DBusServiceName = QStringLiteral("org.projectatomic.rpmostree1");

RpmOstreeTransaction::RpmOstreeTransaction(QObject *parent,
                                           AbstractResource *resource,
                                           OrgProjectatomicRpmostree1SysrootInterface *interface,
                                           Operation operation,
                                           QString arg)
    : Transaction(parent, resource, Transaction::Role::InstallRole, {})
    , m_operation(operation)
    , m_cancelled(false)
    , m_arg(arg)
    , m_interface(interface)
{
    setStatus(Status::SetupStatus);

    // This should never happen. We need a reference to the DBus interface to be
    // able to cancel a running transaction.
    if (interface == nullptr) {
        qWarning() << "rpm-ostree-backend: Error: No DBus interface provided. Please file a bug.";
        passiveMessage("rpm-ostree-backend: Error: No DBus interface provided. Please file a bug.");
        setStatus(Status::CancelledStatus);
        return;
    }

    // Arguments to pass to rpm-ostree command line
    QStringList args = {};

    // Make sure we are asking for a supported operation and set up arguments
    switch (operation) {
    case Operation::CheckForUpdate:
        qInfo() << "rpm-ostree-backend: Starting transaction to check for updates";
        args.append({QStringLiteral("update"), QStringLiteral("--check")});
        break;
    case Operation::DownloadOnly:
        qInfo() << "rpm-ostree-backend: Starting transaction to only download updates";
        args.append({QStringLiteral("update"), QStringLiteral("--download-only ")});
        break;
    case Operation::Update:
        qInfo() << "rpm-ostree-backend: Starting transaction to update";
        args.append({QStringLiteral("update")});
        break;
    case Operation::Rebase:
        // This should never happen
        if (arg.isEmpty()) {
            qWarning() << "rpm-ostree-backend: Error: Can not rebase to an empty ref. Please file a bug.";
            passiveMessage("rpm-ostree-backend: Error: Can not rebase to an empty ref. Please file a bug.");
            setStatus(Status::CancelledStatus);
            return;
        }
        qInfo() << "rpm-ostree-backend: Starting transaction to rebase to:" << arg;
        args.append({QStringLiteral("rebase"), arg});
        break;
    case Operation::Unknown:
        // This is a transaction started externally to Discover. We'll just display
        // it as best as we can.
        qInfo() << "rpm-ostree-backend: Creating a transaction for an operation not started by Discover";
        setupExternalTransaction();
        return;
        break;
    default:
        // This should never happen
        qWarning() << "rpm-ostree-backend: Error: Unknown operation requested. Please file a bug.";
        passiveMessage("rpm-ostree-backend: Error: Unknown operation requested. Please file a bug.");
        setStatus(Status::CancelledStatus);
        return;
    }

    // Directly run the rpm-ostree command via QProcess
    QProcess *m_process = new QProcess(this);

    // Store stderr output for later
    connect(m_process, &QProcess::readyReadStandardError, [this, m_process]() {
        QByteArray message = m_process->readAllStandardError();
        qWarning() << "rpm-ostree-backend: Error message from rpm-ostree:" << message;
        m_stderr += message;
    });

    // Store stdout output for later and process it to fake progress
    connect(m_process, &QProcess::readyReadStandardOutput, [this, m_process]() {
        QByteArray message = m_process->readAllStandardOutput();
        qInfo() << "rpm-ostree:" << message;
        m_stdout += message;
        fakeProgress();
    });

    // Process the result of the transaction once rpm-ostree is done
    connect(m_process,
            QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            [this, m_process, args, operation](int exitCode, QProcess::ExitStatus exitStatus) {
                m_process->deleteLater();
                if (exitStatus != QProcess::NormalExit) {
                    if (m_cancelled) {
                        // If the user requested the transaction to be cancelled
                        // then we don't need to show any error.
                        qWarning() << "rpm-ostree-backend: Transaction cancelled: rpm-ostree " << args;
                    } else {
                        // The transaction was cancelled unexpectedly so let's
                        // display the error to the user.
                        qWarning() << "rpm-ostree-backend: Error while calling: rpm-ostree " << args;
                        qWarning() << "rpm-ostree-backend:" << m_stderr;
                        passiveMessage(i18n("rpm-ostree transaction failed with:") + "\n" + m_stderr);
                    }
                    setStatus(Status::CancelledStatus);
                    return;
                }
                if (exitCode != 0) {
                    qInfo() << "rpm-ostree-backend: Return sucessfully with a non zero return code:" << exitCode;
                    setStatus(Status::DoneWithErrorStatus);
                    return;
                }

                qInfo() << "rpm-ostree-backend: Return sucessfully with a zero return code";
                switch (operation) {
                case Operation::CheckForUpdate: {
                    // Look for new version in rpm-ostree stdout
                    QString newVersion, line;
                    QString output = QString(m_stdout);
                    QTextStream stream(&output);
                    while (stream.readLineInto(&line)) {
                        if (line.contains(QLatin1String("Version: "))) {
                            newVersion = line;
                            break;
                        }
                    }
                    if (!newVersion.isEmpty()) {
                        newVersion = newVersion.trimmed();
                        newVersion.remove(0, QStringLiteral("Version: ").length());
                        newVersion.remove(newVersion.size() - QStringLiteral(" (XXXX-XX-XXTXX:XX:XXZ)").length(), newVersion.size() - 1);
                        qInfo() << "rpm-ostree-backend: Found new version:" << newVersion;
                        Q_EMIT newVersionFound(newVersion);
                    }
                    break;
                }
                case Operation::DownloadOnly:
                    // Nothing to do here
                    break;
                case Operation::Update:
                    // Refresh ressources (deployments) and update state
                    Q_EMIT deploymentsUpdated();
                    break;
                case Operation::Rebase:
                    // Refresh ressources (deployments) and update state
                    Q_EMIT deploymentsUpdated();
                    break;
                case Operation::Unknown:
                default:
                    // This should never happen
                    qWarning() << "rpm-ostree-backend: Error: Unknown operation requested. Please file a bug.";
                    passiveMessage("rpm-ostree-backend: Error: Unknown operation requested. Please file a bug.");
                }
                setStatus(Status::DoneStatus);
            });

    // Effectivly start the rpm-ostree transaction
    auto prog = QStringLiteral("rpm-ostree");
    m_process->start(prog, args);

    // Setup a status that matches the transaction type
    switch (operation) {
    case Operation::CheckForUpdate:
    case Operation::DownloadOnly:
        setStatus(Status::DownloadingStatus);
        break;
    case Operation::Update:
    case Operation::Rebase:
        setStatus(Status::CommittingStatus);
        break;
    case Operation::Unknown:
    default:
        // This should never happen
        qWarning() << "rpm-ostree-backend: Error: Unknown operation requested. Please file a bug.";
        passiveMessage("rpm-ostree-backend: Error: Unknown operation requested. Please file a bug.");
        setStatus(Status::CancelledStatus);
        return;
    }
}

RpmOstreeTransaction::~RpmOstreeTransaction()
{
}

void RpmOstreeTransaction::setupExternalTransaction()
{
    // Create a timer to periodically look for updates on the transaction
    m_timer = new QTimer(this);
    m_timer->setSingleShot(true);
    m_timer->setInterval(2000);

    // Update transaction status
    connect(m_timer, &QTimer::timeout, [this]() {
        // Is the transaction finished?
        qDebug() << "rpm-ostree-backend: External transaction update timer triggered";
        QString transaction = m_interface->activeTransactionPath();
        if (transaction.isEmpty()) {
            qInfo() << "rpm-ostree-backend: External transaction finished";
            Q_EMIT deploymentsUpdated();
            setStatus(Status::DoneStatus);
            return;
        }

        // Read status and fake progress
        QStringList transactionInfo = m_interface->activeTransaction();
        if (transactionInfo.length() != 3) {
            qInfo() << "rpm-ostree-backend: External transaction:" << transactionInfo;
        } else {
            qInfo() << "rpm-ostree-backend: External transaction '" << transactionInfo.at(0) << "' requested by '" << transactionInfo.at(1);
        }
        fakeProgress();

        // Restart the timer
        m_timer->start();
    });

    // Setup status, fake progress and start the timer
    setStatus(Transaction::Status::DownloadingStatus);
    fakeProgress();
    m_timer->start();
}

void RpmOstreeTransaction::fakeProgress()
{
    int progress = this->progress();
    if (progress < 50) {
        setProgress(progress + 5);
    } else if (progress < 90) {
        setProgress(progress + 2);
    } else if (progress < 99) {
        setProgress(progress + 1);
    }
}

void RpmOstreeTransaction::cancel()
{
    qInfo() << "rpm-ostree-backend: Cancelling current transaction";
    passiveMessage(i18n("Cancelling rpm-ostree transaction. This may take some time. Please wait."));

    // Cancel directly using the DBus interface to work in all casesn whether we
    // sstarted the transaction or if it's an externally started one.
    QString transaction = m_interface->activeTransactionPath();
    QDBusConnection peerConnection = QDBusConnection::connectToPeer(transaction, TransactionConnection);
    OrgProjectatomicRpmostree1TransactionInterface transactionInterface(DBusServiceName, QStringLiteral("/"), peerConnection, this);
    auto reply = transactionInterface.Cancel();

    // Cancelled marker that is used to avoid displaying an error message to the
    // user when they asked to cancel a transaction.
    m_cancelled = true;

    QDBusPendingCallWatcher *callWatcher = new QDBusPendingCallWatcher(reply, this);
    connect(callWatcher, &QDBusPendingCallWatcher::finished, [callWatcher]() {
        callWatcher->deleteLater();
        QDBusConnection::disconnectFromPeer(TransactionConnection);
    });
}

void RpmOstreeTransaction::proceed()
{
    qInfo() << "rpm-ostree-backend: proceed";
}

QString RpmOstreeTransaction::name() const
{
    switch (m_operation) {
    case Operation::CheckForUpdate:
        return i18n("Checking for a system update");
        break;
    case Operation::DownloadOnly:
        return i18n("Downloading system update");
        break;
    case Operation::Update:
        return i18n("Updating the system");
        break;
    case Operation::Rebase:
        return i18n("Updating to the next major version");
        break;
    case Operation::Unknown:
        return i18n("Transaction in progress (started outside of Discover)");
        break;
    default:
        break;
    }
    // This should never happen
    qWarning() << "rpm-ostree-backend: Error: Unknown operation requested. Please file a bug.";
    return i18n("Unknown");
}

QVariant RpmOstreeTransaction::icon() const
{
    return QStringLiteral("application-x-rpm");
}
