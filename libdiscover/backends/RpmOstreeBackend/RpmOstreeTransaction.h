/*
 *   SPDX-FileCopyrightText: 2021 Mariam Fahmy Sobhy <mariamfahmy66@gmail.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */
 
#ifndef OSTREERPMTRANSACTION_H
#define OSTREERPMTRANSACTION_H

#include "RpmOstreeDBusInterface.h"

#include <Transaction/Transaction.h>

class RpmOstreeTransaction : public Transaction
{
    Q_OBJECT
public:
    RpmOstreeTransaction(QObject *parent,
                         AbstractResource *resource,
                         QString path,
                         Transaction::Role role = Transaction::Role::InstallRole,
                         const AddonList &addons = {});
    ~RpmOstreeTransaction();

    Q_SCRIPTABLE void cancel() override;
    Q_SCRIPTABLE void proceed() override;

    QString name() const override;
    QVariant icon() const override;

public Q_SLOTS:
    void DownloadProgress(const QStringList &time,
                          const QVariantList &outstanding,
                          const QVariantList &metadata,
                          const QVariantList &delta,
                          const QVariantList &content,
                          const QVariantList &transfer);
    void Finished(bool success, const QString &error_message);
    void Message(const QString &text);
    void PercentProgress(const QString &text, uint percentage);
    void ProgressEnd();
    void SignatureProgress(const QVariantList &signature, const QString &commit);
    void TaskBegin(const QString &text);
    void TaskEnd(const QString &text);

private:
    QString m_path;
    OrgProjectatomicRpmostree1TransactionInterface *m_interface;
    QString m_step;
};

#endif
