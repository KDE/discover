/***************************************************************************
 *   Copyright © 2012 Jonathan Thomas <echidnaman@kubuntu.org>             *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU General Public License as        *
 *   published by the Free Software Foundation; either version 2 of        *
 *   the License or (at your option) version 3 or any later version        *
 *   accepted by the membership of KDE e.V. (or its successor approved     *
 *   by the membership of KDE e.V.), which shall act as a proxy            *
 *   defined in Section 14 of version 3 of the license.                    *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>. *
 ***************************************************************************/

#ifndef TRANSACTIONMODEL_H
#define TRANSACTIONMODEL_H

#include <QAbstractListModel>

#include "Transaction.h"

#include "discovercommon_export.h"

class DISCOVERCOMMON_EXPORT TransactionModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)
public:
    enum Roles {
        TransactionRoleRole = Qt::UserRole,
        TransactionStatusRole,
        CancellableRole,
        ProgressRole,
        StatusTextRole,
        ResourceRole
    };

    explicit TransactionModel(QObject *parent = nullptr);
    static TransactionModel *global();

    // Reimplemented from QAbstractListModel
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    Transaction *transactionFromIndex(const QModelIndex &index) const;
    Q_SCRIPTABLE Transaction *transactionFromResource(AbstractResource *resource) const;
    QModelIndex indexOf(Transaction *trans) const;
    QModelIndex indexOf(AbstractResource *res) const;

    void addTransaction(Transaction *trans);
    void cancelTransaction(Transaction *trans);
    void removeTransaction(Transaction *trans);

private:
    QVector<Transaction *> m_transactions;
    
Q_SIGNALS:
    void startingFirstTransaction();
    void lastTransactionFinished();
    void transactionAdded(Transaction *trans);
    void transactionCancelled(Transaction *trans);
    void transactionRemoved(Transaction* trans);
    void countChanged();

private Q_SLOTS:
    void transactionChanged();
};

#endif // TRANSACTIONMODEL_H
