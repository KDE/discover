/*
 *   SPDX-FileCopyrightText: 2020 Alexey Min <alexey.min@gmail.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#ifndef ALPINEAPKTRANSACTION_H
#define ALPINEAPKTRANSACTION_H

#include <Transaction/Transaction.h>

class AlpineApkBackend;
class AlpineApkResource;

class AlpineApkTransaction : public Transaction
{
Q_OBJECT
public:
    AlpineApkTransaction(AlpineApkResource *res, Role role);
    AlpineApkTransaction(AlpineApkResource *res, const AddonList &list, Role role);

    void cancel() override;
    void proceed() override;

private Q_SLOTS:
    void startTransaction();
    void finishTransactionOK();
    void finishTransactionWithError(const QString &errMsg);

private:
    AlpineApkResource *m_resource;
    AlpineApkBackend *m_backend;
};

#endif // ALPINEAPKTRANSACTION_H
