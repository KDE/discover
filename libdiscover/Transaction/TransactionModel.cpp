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

#include "TransactionModel.h"

// Qt includes
#include <QtCore/QMetaProperty>
#include <KLocalizedString>

// Own includes
#include "resources/AbstractResource.h"

Q_GLOBAL_STATIC(TransactionModel, globalTransactionModel)

TransactionModel *TransactionModel::global()
{
    return globalTransactionModel;
}

TransactionModel::TransactionModel(QObject *parent)
    : QAbstractListModel(parent)
{
    connect(this, &QAbstractItemModel::rowsInserted, this, &TransactionModel::countChanged);
    connect(this, &QAbstractItemModel::rowsRemoved, this, &TransactionModel::countChanged);
}

QHash< int, QByteArray > TransactionModel::roleNames() const
{
    QHash<int, QByteArray> roles = QAbstractItemModel::roleNames();
    roles[TransactionRoleRole] = "transactionRole";
    roles[TransactionStatusRole] = "status";
    roles[CancellableRole] = "cancellable";
    roles[ProgressRole] = "progress";
    roles[StatusTextRole] = "statusText";
    roles[ResourceRole] = "resource";
    return roles;
}

int TransactionModel::rowCount(const QModelIndex &parent) const
{
    // Root element parents all children
    if (!parent.isValid())
        return m_transactions.size();

    // Child elements have no children themselves
    return 0;
}

QVariant TransactionModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    Transaction *trans = transactionFromIndex(index);
    switch (role) {
    case TransactionRoleRole:
        return trans->role();
    case TransactionStatusRole:
        return trans->status();
    case CancellableRole:
        return trans->isCancellable();
    case ProgressRole:
        return trans->progress();
    case StatusTextRole:
        switch (trans->status()) {
        case Transaction::SetupStatus:
            return i18nc("@info:status", "Starting");
        case Transaction::QueuedStatus:
            return i18nc("@info:status", "Waiting");
        case Transaction::DownloadingStatus:
            return i18nc("@info:status", "Downloading");
        case Transaction::CommittingStatus:
            switch (trans->role()) {
            case Transaction::InstallRole:
                return i18nc("@info:status", "Installing");
            case Transaction::RemoveRole:
                return i18nc("@info:status", "Removing");
            case Transaction::ChangeAddonsRole:
                return i18nc("@info:status", "Changing Addons");
            }
            break;
        case Transaction::DoneStatus:
            return i18nc("@info:status", "Done");
        }
        break;
    case ResourceRole:

        return qVariantFromValue<QObject*>(trans->resource());
    }

    return QVariant();
}

Transaction *TransactionModel::transactionFromIndex(const QModelIndex &index) const
{
    Transaction *trans = nullptr;

    if (index.isValid() && index.row() < m_transactions.size())
        trans = m_transactions.at(index.row());

    return trans;
}

Transaction *TransactionModel::transactionFromResource(AbstractResource *resource) const
{
    Transaction *ret = nullptr;

    Q_FOREACH (Transaction *trans, m_transactions) {
        if (trans->resource() == resource) {
            ret = trans;
            break;
        }
    }

    return ret;
}

QModelIndex TransactionModel::indexOf(Transaction *trans) const
{
    int row = m_transactions.indexOf(trans);
    QModelIndex ret = index(row);
    Q_ASSERT(!trans || ret.isValid());
    return ret;
}

QModelIndex TransactionModel::indexOf(AbstractResource *res) const
{
    Transaction *trans = transactionFromResource(res);

    return indexOf(trans);
}

void TransactionModel::addTransaction(Transaction *trans)
{
    if (m_transactions.contains(trans))
        return;

    if (m_transactions.isEmpty())
        emit startingFirstTransaction();

    int before = m_transactions.size();
    beginInsertRows(QModelIndex(), before, before + 1);
    m_transactions.append(trans);
    endInsertRows();

    connect(trans, &Transaction::statusChanged, this, [this](){ transactionChanged(StatusTextRole); });
    connect(trans, &Transaction::cancellableChanged, this, [this](){ transactionChanged(CancellableRole); });
    connect(trans, &Transaction::progressChanged, this, [this](){ transactionChanged(ProgressRole); Q_EMIT progressChanged(); });

    emit transactionAdded(trans);
}

void TransactionModel::cancelTransaction(Transaction *trans)
{
    removeTransaction(trans);

    emit transactionCancelled(trans);
}

void TransactionModel::removeTransaction(Transaction *trans)
{
    Q_ASSERT(trans);
    int r = indexOf(trans).row();
    Q_ASSERT(r>=0);

    beginRemoveRows(QModelIndex(), r, r);
    m_transactions.removeAt(r);
    endRemoveRows();
    emit transactionRemoved(trans);
    if (m_transactions.isEmpty())
        emit lastTransactionFinished(); 
}

void TransactionModel::transactionChanged(int role)
{
    Transaction *trans = qobject_cast<Transaction *>(sender());
    QModelIndex transIdx = indexOf(trans);
    emit dataChanged(transIdx, transIdx, {role});
}

int TransactionModel::progress() const
{
    int sum = 0;
    int count = 0;
    foreach(Transaction* t, m_transactions) {
        if (t->isActive()) {
            sum += t->progress();
            ++count;
        }
    }
    return sum / count;
}
