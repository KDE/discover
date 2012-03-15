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

#include "libmuonprivate_export.h"

class Application;
class ApplicationBackend;
class Transaction;
class TransactionListener;

class MUONPRIVATE_EXPORT TransactionModel : public QAbstractListModel
{
    Q_OBJECT
public:
    explicit TransactionModel(QObject *parent = 0);

    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;
    int rowCount(const QModelIndex &parent = QModelIndex()) const;

    void setBackend(ApplicationBackend* appBackend);
    void addTransactions(const QList<Transaction *> &trans);

private:
    ApplicationBackend *m_appBackend;
    QList<TransactionListener *> m_transactions;

    void removeTransaction(TransactionListener *listener);
    
private slots:
    void addTransaction(Transaction *trans);
    void removeTransaction(Application *app);
    void externalUpdate();
    void clear();

signals:
    void lastTransactionCancelled();
};

#endif // TRANSACTIONMODEL_H
