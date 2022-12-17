/*
 *   SPDX-FileCopyrightText: 2010 Jonathan Thomas <echidnaman@kubuntu.org>
 *   SPDX-FileCopyrightText: 2012 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include <QObject>

#include "Transaction.h"
#include "discovercommon_export.h"

class AbstractResource;

class DISCOVERCOMMON_EXPORT TransactionListener : public QObject
{
    Q_OBJECT
    Q_PROPERTY(AbstractResource *resource READ resource WRITE setResource NOTIFY resourceChanged)
    Q_PROPERTY(Transaction *transaction READ transaction WRITE setTransaction NOTIFY transactionChanged)
    Q_PROPERTY(bool isCancellable READ isCancellable NOTIFY cancellableChanged)
    Q_PROPERTY(bool isActive READ isActive NOTIFY isActiveChanged)
    Q_PROPERTY(QString statusText READ statusText NOTIFY statusTextChanged)
    Q_PROPERTY(int progress READ progress NOTIFY progressChanged)
public:
    explicit TransactionListener(QObject *parent = nullptr);

    AbstractResource *resource() const
    {
        return m_resource;
    }
    Transaction *transaction() const
    {
        return m_transaction;
    }
    bool isCancellable() const;
    bool isActive() const;
    QString statusText() const;
    int progress() const;

    Q_SCRIPTABLE void cancel();

    void setResource(AbstractResource *resource);
    void setTransaction(Transaction *trans);

private:
    void setResourceInternal(AbstractResource *resource);

    AbstractResource *m_resource = nullptr;
    Transaction *m_transaction = nullptr;

private Q_SLOTS:
    void transactionAdded(Transaction *trans);
    void transactionStatusChanged(Transaction::Status status);

Q_SIGNALS:
    void resourceChanged();
    void cancellableChanged();
    void isActiveChanged();
    void statusTextChanged();
    void cancelled();
    void progressChanged();
    void transactionChanged(Transaction *transaction);
};
