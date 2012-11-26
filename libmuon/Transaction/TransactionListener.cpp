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
#include "resources/AbstractResourcesBackend.h"
#include "Transaction.h"

#include <KLocalizedString>
#include <KDebug>

TransactionListener::TransactionListener(QObject* parent)
    : QObject(parent)
    , m_backend(0)
    , m_resource(0)
    , m_progress(0)
    , m_downloading(false)
{}

void TransactionListener::setBackend(AbstractResourcesBackend* backend)
{
    if(m_backend) {
        disconnect(m_backend, SIGNAL(transactionsEvent(TransactionStateTransition,Transaction*)),
                this, SLOT(workerEvent(TransactionStateTransition,Transaction*)));
        disconnect(m_backend, SIGNAL(transactionCancelled(Transaction*)),
                this, SLOT(transactionCancelled(Transaction*)));
        disconnect(m_backend, SIGNAL(transactionRemoved(Transaction*)),
                this, SLOT(transactionRemoved(Transaction*)));
    }
    
    m_backend = backend;
    if(backend) {
        // Catch already-begun downloads. If the state is something else, we won't
        // care because we won't handle it
        init();
        
        connect(m_backend, SIGNAL(transactionsEvent(TransactionStateTransition,Transaction*)),
                this, SLOT(workerEvent(TransactionStateTransition,Transaction*)));
        connect(m_backend, SIGNAL(transactionCancelled(Transaction*)),
                this, SLOT(transactionCancelled(Transaction*)));
        connect(m_backend, SIGNAL(transactionRemoved(Transaction*)),
                    this, SLOT(transactionRemoved(Transaction*)));
    }
}

void TransactionListener::init()
{
    if(!m_backend)
        return;
    
    QPair<TransactionStateTransition, Transaction *> workerState = m_backend->currentTransactionState();
    if(!workerState.second)
        return;

    workerEvent(workerState.first, workerState.second);

    foreach (Transaction *transaction, m_backend->transactions()) {
        if (transaction->resource() == m_resource) {
            emit running(true);
            showTransactionState(transaction);
        }
    }
}

bool TransactionListener::isActive() const
{
    if(!m_backend)
        return false;

    foreach (Transaction *transaction, m_backend->transactions()) {
        if (transaction->resource() == m_resource) {
            return transaction->state()!=TransactionState::DoneState;
        }
    }
    return false;
}

bool TransactionListener::isDownloading() const
{
    return m_backend && m_downloading;
}

void TransactionListener::workerEvent(TransactionStateTransition event, Transaction *transaction)
{
    Q_ASSERT(transaction);
    if (m_resource != transaction->resource()) {
        return;
    }

    switch (event) {
    case StartedDownloading:
        m_comment = i18nc("@info:status", "Downloading");
        m_progress = 0;
        connect(m_backend, SIGNAL(transactionProgressed(Transaction*,int)),
                this, SLOT(updateProgress(Transaction*,int)));
        emit running(true);
        emit commentChanged();
        emit progressChanged();
        setDownloading(true);
        break;
    case FinishedDownloading:
        disconnect(m_backend, SIGNAL(transactionProgressed(Transaction*,int)),
                   this, SLOT(updateProgress(Transaction*,int)));
        setDownloading(false);
        break;
    case StartedCommitting:
        setStateComment(transaction);
        connect(m_backend, SIGNAL(transactionProgressed(Transaction*,int)),
                this, SLOT(updateProgress(Transaction*,int)));
        break;
    case FinishedCommitting:
        emit running(false);
        disconnect(m_backend, SIGNAL(transactionProgressed(Transaction*,int)),
                   this, SLOT(updateProgress(Transaction*,int)));
        break;
    }
}

void TransactionListener::updateProgress(Transaction *transaction, int percentage)
{
    if (m_resource == transaction->resource()) {
        m_progress = percentage;
        emit progressChanged();

        if (percentage == 100) {
            m_comment = i18nc("@info:status Progress text when done", "Done");
            emit commentChanged();
        }
    }
}

void TransactionListener::showTransactionState(Transaction *transaction)
{
    switch (transaction->state()) {
        case QueuedState:
            m_comment = i18nc("@info:status Progress text when waiting", "Waiting");
            emit commentChanged();
            break;
        case RunningState:
            setStateComment(transaction);
            break;
        case DoneState:
            m_comment = i18nc("@info:status Progress text when done", "Done");
            m_progress = 100;
            emit progressChanged();
            emit commentChanged();
            break;
        default:
            break;
    }
}

void TransactionListener::setStateComment(Transaction* transaction)
{
    switch(transaction->action()) {
        case InstallApp:
            m_comment = i18nc("@info:status", "Installing");
            emit commentChanged();
            emit running(true);
            break;
        case ChangeAddons:
            m_comment = i18nc("@info:status", "Changing Addons");
            emit commentChanged();
            break;
        case RemoveApp:
            m_comment = i18nc("@info:status", "Removing");
            emit commentChanged();
            break;
        default:
            break;
    }
}

QString TransactionListener::comment() const
{
    return m_comment;
}

int TransactionListener::progress() const
{
    return m_progress;
}

void TransactionListener::transactionCancelled(Transaction* t)
{
    if(t->resource()!=m_resource)
        return;
    emit running(false);
    setDownloading(false);
    emit cancelled();
}

void TransactionListener::setResource(AbstractResource* app)
{
    if(m_resource!=app) {
        m_resource = app;
        init();
        emit resourceChanged();
    }
}

AbstractResource* TransactionListener::resource() const
{
    return m_resource;
}

AbstractResourcesBackend* TransactionListener::backend() const
{
    return m_backend;
}

void TransactionListener::setDownloading(bool b)
{
    m_downloading = b;
    emit downloading(b);
}

void TransactionListener::transactionRemoved(Transaction* t)
{
    if(t && t->resource()==m_resource) {
        emit running(false);
    }
}
