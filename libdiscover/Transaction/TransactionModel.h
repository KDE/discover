/*
 *   SPDX-FileCopyrightText: 2012 Jonathan Thomas <echidnaman@kubuntu.org>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#ifndef TRANSACTIONMODEL_H
#define TRANSACTIONMODEL_H

#include <QAbstractListModel>

#include "Transaction.h"

#include "discovercommon_export.h"

class DISCOVERCOMMON_EXPORT TransactionModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int progress READ progress NOTIFY progressChanged)
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)
public:
    enum Roles {
        TransactionRoleRole = Qt::UserRole,
        TransactionStatusRole,
        CancellableRole,
        ProgressRole,
        StatusTextRole,
        ResourceRole,
        TransactionRole
    };

    explicit TransactionModel(QObject *parent = nullptr);
    static TransactionModel *global();

    // Reimplemented from QAbstractListModel
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    Q_SCRIPTABLE Transaction *transactionFromResource(AbstractResource *resource) const;
    QModelIndex indexOf(Transaction *trans) const;
    QModelIndex indexOf(AbstractResource *res) const;

    void addTransaction(Transaction *trans);
    void removeTransaction(Transaction *trans);

    bool contains(Transaction* transaction) const { return m_transactions.contains(transaction); }
    int progress() const;
    QVector<Transaction *> transactions() const { return m_transactions; }

private:
    QVector<Transaction *> m_transactions;
    
Q_SIGNALS:
    void startingFirstTransaction();
    void lastTransactionFinished();
    void transactionAdded(Transaction *trans);
    void transactionRemoved(Transaction* trans);
    void countChanged();
    void progressChanged();
    void proceedRequest(Transaction* transaction, const QString &title, const QString &description);

private Q_SLOTS:
    void transactionChanged(int role);
};

#endif // TRANSACTIONMODEL_H
