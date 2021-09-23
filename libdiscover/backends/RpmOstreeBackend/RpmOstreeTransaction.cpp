/*
 *   SPDX-FileCopyrightText: 2021 Mariam Fahmy Sobhy <mariamfahmy66@gmail.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "RpmOstreeTransaction.h"

#include <KLocalizedString>

static const QString DBusServiceName = QStringLiteral("org.projectatomic.rpmostree1");
static const QString TransactionConnection = QStringLiteral("discover_transaction");

RpmOstreeTransaction::RpmOstreeTransaction(QObject *parent, AbstractResource *resource, QString path, Transaction::Role role, const AddonList &addons)
    : Transaction(parent, resource, role, addons)
    , m_path(path)
{
    Q_UNUSED(role);
    Q_UNUSED(addons);
    setStatus(Status::SetupStatus);
    qDebug() << "rpm-ostree-backend: Starting new transaction at:" << m_path;

    auto connection = QDBusConnection::connectToPeer(m_path, TransactionConnection);
    m_interface = new OrgProjectatomicRpmostree1TransactionInterface(DBusServiceName, QStringLiteral("/"), connection, this);

    Q_ASSERT(QObject::connect(m_interface, &OrgProjectatomicRpmostree1TransactionInterface::DownloadProgress, this, &RpmOstreeTransaction::DownloadProgress));
    Q_ASSERT(QObject::connect(m_interface, &OrgProjectatomicRpmostree1TransactionInterface::Finished, this, &RpmOstreeTransaction::Finished));
    Q_ASSERT(QObject::connect(m_interface, &OrgProjectatomicRpmostree1TransactionInterface::Message, this, &RpmOstreeTransaction::Message));
    Q_ASSERT(QObject::connect(m_interface, &OrgProjectatomicRpmostree1TransactionInterface::PercentProgress, this, &RpmOstreeTransaction::PercentProgress));
    Q_ASSERT(QObject::connect(m_interface, &OrgProjectatomicRpmostree1TransactionInterface::ProgressEnd, this, &RpmOstreeTransaction::ProgressEnd));
    Q_ASSERT(QObject::connect(m_interface, &OrgProjectatomicRpmostree1TransactionInterface::SignatureProgress, this, &RpmOstreeTransaction::SignatureProgress));
    Q_ASSERT(QObject::connect(m_interface, &OrgProjectatomicRpmostree1TransactionInterface::TaskBegin, this, &RpmOstreeTransaction::TaskBegin));
    Q_ASSERT(QObject::connect(m_interface, &OrgProjectatomicRpmostree1TransactionInterface::TaskEnd, this, &RpmOstreeTransaction::TaskEnd));

    QDBusPendingReply<bool> reply = m_interface->Start();
    reply.waitForFinished();
    if (reply.isError()) {
        qWarning() << "rpm-ostree-backend: Error while starting transaction:" << reply.error();
    }
    if (!reply.value()) {
        qWarning() << "rpm-ostree-backend: Something else started this transaction before us. This is likely a bug.";
    }
    setStatus(Status::QueuedStatus);
}

RpmOstreeTransaction::~RpmOstreeTransaction()
{
    if (m_interface != nullptr) {
        delete m_interface;
        m_interface = nullptr;
    }
    QDBusConnection::disconnectFromPeer(TransactionConnection);
}

void RpmOstreeTransaction::DownloadProgress(const QStringList &time,
                                            const QVariantList &outstanding,
                                            const QVariantList &metadata,
                                            const QVariantList &delta,
                                            const QVariantList &content,
                                            const QVariantList &transfer)
{
    setStatus(Transaction::Status::DownloadingStatus);
    qInfo() << "rpm-ostree-backend: DownloadProgress:" << time << outstanding << metadata << delta << content << transfer;
}

void RpmOstreeTransaction::Finished(bool success, const QString &error_message)
{
    qInfo() << "rpm-ostree-backend: Transation finished:" << success << error_message;
    success ? setStatus(Status::DoneStatus) : setStatus(Status::DoneWithErrorStatus);
}

void RpmOstreeTransaction::Message(const QString &text)
{
    qInfo() << "rpm-ostree-backend: Message:" << text;
    emit(passiveMessage(text));
}

void RpmOstreeTransaction::PercentProgress(const QString &text, uint percentage)
{
    qInfo() << "rpm-ostree-backend: PercentProgress:" << text << percentage;
    m_step = text;
    setProgress(percentage);
}

void RpmOstreeTransaction::ProgressEnd()
{
    qInfo() << "rpm-ostree-backend: ProgressEnd";
}

void RpmOstreeTransaction::SignatureProgress(const QVariantList &signature, const QString &commit)
{
    qInfo() << "rpm-ostree-backend: SignatureProgress:" << signature << commit;
}

void RpmOstreeTransaction::TaskBegin(const QString &text)
{
    qInfo() << "rpm-ostree-backend: Transation: Begining task:" << text;
}

void RpmOstreeTransaction::TaskEnd(const QString &text)
{
    qInfo() << "rpm-ostree-backend: Transation: Ending task:" << text;
}

void RpmOstreeTransaction::cancel()
{
    qInfo() << "rpm-ostree-backend: Cancelling current transaction";
    m_interface->Cancel().waitForFinished();
    setStatus(Status::CancelledStatus);
}

void RpmOstreeTransaction::proceed()
{
    qInfo() << "rpm-ostree-backend: proceed";
}

QString RpmOstreeTransaction::name() const
{
    return i18n("Current rpm-ostree transaction in progress:") + m_step;
}

QVariant RpmOstreeTransaction::icon() const
{
    return QStringLiteral("application-x-rpm");
}
