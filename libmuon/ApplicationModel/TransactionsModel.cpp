/***************************************************************************
 *   Copyright © 2012 Aleix Pol Gonzalez <aleixpol@blue-systems.com>       *
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

#include "TransactionsModel.h"
#include <ApplicationBackend.h>
#include <Transaction/Transaction.h>
#include <Application.h>
#include <QDebug>

TransactionsModel::TransactionsModel(QObject* parent)
    : QAbstractListModel(parent)
    , m_backend(0)
{}

int TransactionsModel::rowCount(const QModelIndex& parent) const
{
    if(parent.isValid())
        return 0;
    return m_transactions.size();
}

QString transactionState(Transaction* t)
{
    switch(t->state()) {
        case QueuedState:           return i18n("Waiting...");
        case RunningState:
            switch(t->action()) {
                case InstallApp:    return i18n("Installing...");
                case RemoveApp:     return i18n("Removing...");
                case ChangeAddons:  return i18n("Changing Addons...");
                default:            return i18n("Working...");
            }
        case DoneState:             return i18n("Done");
        default:                    return i18n("Working...");
    }
}

QVariant TransactionsModel::data(const QModelIndex& index, int role) const
{
    if(!index.isValid() || index.row()>=rowCount())
        return QVariant();
    
    Transaction* t = m_transactions[index.row()];
    switch(role) {
        case Qt::DisplayRole:
            return t->application()->name();
        case Qt::DecorationRole:
            return t->application()->icon();
        case Qt::ToolTip:
            return transactionState(t);
    }
    
    return QVariant();
}

ApplicationBackend* TransactionsModel::backend() const
{
    return m_backend;
}

void TransactionsModel::setBackend(ApplicationBackend* b)
{
    if(m_backend) {
        disconnect(b, SIGNAL(reloadStarted()), this, SLOT(clearTransactions()));
        disconnect(b, SIGNAL(transactionRemoved(Transaction*)), this, SLOT(removeTransaction(Transaction*)));
        disconnect(b, SIGNAL(transactionAdded(Transaction*)), this, SLOT(addTransaction(Transaction*)));
        disconnect(b, SIGNAL(workerEvent(QApt::WorkerEvent,Transaction*)), this, SLOT(transactionChanged(QApt::WorkerEvent,Transaction*)));
    }
    
    m_backend = b;
    m_transactions.clear();
    if(b) {
        m_transactions = b->transactions();
        connect(b, SIGNAL(reloadStarted()), SLOT(clearTransactions()));
        connect(b, SIGNAL(transactionRemoved(Transaction*)), SLOT(removeTransaction(Transaction*)));
        connect(b, SIGNAL(transactionAdded(Transaction*)), SLOT(addTransaction(Transaction*)));
        connect(b, SIGNAL(workerEvent(QApt::WorkerEvent,Transaction*)), SLOT(transactionChanged(QApt::WorkerEvent,Transaction*)));
    }
}

void TransactionsModel::clearTransactions()
{
    beginResetModel();
    m_transactions.clear();
    endResetModel();
}

void TransactionsModel::removeTransaction(Transaction* t)
{
    int idx = m_transactions.indexOf(t);
    if(idx>=0) {
        beginRemoveRows(QModelIndex(), idx, idx);
        m_transactions.removeAt(idx);
        endRemoveRows();
    }
}

void TransactionsModel::addTransaction(Transaction* t)
{
    beginInsertRows(QModelIndex(), 0, 0);
    m_transactions.prepend(t);
    endInsertRows();
}

void TransactionsModel::transactionChanged(QApt::WorkerEvent , Transaction* t)
{
    int idx = m_transactions.indexOf(t);
    if(idx>=0) {
        QModelIndex mindex=index(idx);
        emit dataChanged(mindex, mindex);
    }
}
