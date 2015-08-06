/***************************************************************************
 *   Copyright © 2010 Jonathan Thomas <echidnaman@kubuntu.org>             *
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

#include "TransactionListener.h"

#include "TransactionModel.h"
#include <QMetaProperty>
#include <QDebug>

TransactionListener::TransactionListener(QObject *parent)
    : QObject(parent)
    , m_resource(nullptr)
    , m_transaction(nullptr)
{
    connect(TransactionModel::global(), SIGNAL(transactionAdded(Transaction*)),
            this, SLOT(transactionAdded(Transaction*)));
    connect(TransactionModel::global(), SIGNAL(transactionRemoved(Transaction*)),
            this, SLOT(transactionRemoved(Transaction*)));
    connect(TransactionModel::global(), SIGNAL(transactionCancelled(Transaction*)),
            this, SLOT(transactionCancelled(Transaction*)));
}

AbstractResource *TransactionListener::resource() const
{
    return m_resource;
}

bool TransactionListener::isCancellable() const
{
    return m_transaction && m_transaction->isCancellable();
}

bool TransactionListener::isActive() const
{
    return m_transaction && m_transaction->status() != Transaction::SetupStatus;
}

QString TransactionListener::statusText() const
{
    QModelIndex index = TransactionModel::global()->indexOf(m_resource);

    return index.data(TransactionModel::StatusTextRole).toString();
}

void TransactionListener::setResource(AbstractResource *resource)
{
    if (m_resource == resource)
        return;

    m_resource = resource;

    // Catch already-started transactions
    setTransaction(TransactionModel::global()->transactionFromResource(resource));

    emit resourceChanged();
}

void TransactionListener::transactionAdded(Transaction *trans)
{
    if (trans->resource() != m_resource)
        return;

    setTransaction(trans);
}

class CheckChange
{
public:
    CheckChange(QObject* obj, const QByteArray& prop)
        : m_object(obj)
        , m_prop(obj->metaObject()->property(obj->metaObject()->indexOfProperty(prop)))
        , m_oldValue(m_prop.read(obj))
    {
        Q_ASSERT(obj->metaObject()->indexOfProperty(prop)>=0);
    }

    ~CheckChange() {
        const QVariant newValue = m_prop.read(m_object);
        if (newValue != m_oldValue) {
            QMetaMethod m = m_prop.notifySignal();
            m.invoke(m_object, Qt::DirectConnection);
        }
    }

private:
    QObject* m_object;
    QMetaProperty m_prop;
    QVariant m_oldValue;
};

void TransactionListener::setTransaction(Transaction* trans)
{
    Q_ASSERT(!trans || trans->resource()==m_resource);
    if (m_transaction == trans) {
        return;
    }

    if(m_transaction) {
        disconnect(m_transaction, nullptr, this, nullptr);
    }

    CheckChange change1(this, "isCancellable");
    CheckChange change2(this, "isRunning");
    CheckChange change3(this, "statusText");
    CheckChange change4(this, "progress");

    m_transaction = trans;
    if(m_transaction) {
        connect(m_transaction, SIGNAL(cancellableChanged(bool)),
                this, SIGNAL(cancellableChanged()));
        connect(m_transaction, SIGNAL(statusChanged(Transaction::Status)),
                this, SLOT(transactionStatusChanged(Transaction::Status)));
        connect(m_transaction, SIGNAL(progressChanged(int)),
                this, SIGNAL(progressChanged()));
    }
}

void TransactionListener::transactionStatusChanged(Transaction::Status status)
{
    switch (status) {
    case Transaction::DoneStatus:
        setTransaction(nullptr);
        break;
    case Transaction::QueuedStatus:
        emit runningChanged();
        break;
    default:
        break;
    }

    emit statusTextChanged();
}

void TransactionListener::transactionRemoved(Transaction* trans)
{
    if(m_transaction == trans) {
        setTransaction(nullptr);
    }
}

void TransactionListener::transactionCancelled(Transaction* trans)
{
    if(m_transaction == trans) {
        setTransaction(nullptr);
    }
    emit cancelled();
}

int TransactionListener::progress() const
{
    return m_transaction ? m_transaction->progress() : 0;
}
