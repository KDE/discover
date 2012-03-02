/***************************************************************************
 *   Copyright © 2010 Jonathan Thomas <echidnaman@kubuntu.org>             *
 *   Copyright © 2012 Aleix Pol Gonzalez <aleixpol@kde.org>                *
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
#include "ApplicationBackend.h"
#include "Transaction.h"
#include "Application.h"

#include <KLocalizedString>

TransactionListener::TransactionListener(QObject* parent)
    : QObject(parent)
    , m_appBackend(0)
    , m_app(0)
{}

TransactionListener::~TransactionListener()
{}

void TransactionListener::setBackend(ApplicationBackend* appBackend)
{
    if(m_appBackend) {
        disconnect(m_appBackend, SIGNAL(workerEvent(QApt::WorkerEvent,Transaction*)),
                this, SLOT(workerEvent(QApt::WorkerEvent,Transaction*)));
        disconnect(m_appBackend, SIGNAL(transactionCancelled(Application*)),
                this, SLOT(transactionCancelled(Application*)));
    }
    
    m_appBackend = appBackend;
    // Catch already-begun downloads. If the state is something else, we won't
    // care because we won't handle it
    init();
    
    connect(m_appBackend, SIGNAL(workerEvent(QApt::WorkerEvent,Transaction*)),
            this, SLOT(workerEvent(QApt::WorkerEvent,Transaction*)));
    connect(m_appBackend, SIGNAL(transactionCancelled(Application*)),
            this, SLOT(transactionCancelled(Application*)));
}

void TransactionListener::init()
{
    QPair<QApt::WorkerEvent, Transaction *> workerState = m_appBackend->workerState();
    workerEvent(workerState.first, workerState.second);

    foreach (Transaction *transaction, m_appBackend->transactions()) {
        if (transaction->application() == m_app) {
            emit installing();
            showTransactionState(transaction);
        }
    }
}

bool TransactionListener::isInstalling() const
{
    if(!m_appBackend)
        return false;
    foreach (Transaction *transaction, m_appBackend->transactions()) {
        if (transaction->application() == m_app) {
            return transaction->state()!=TransactionState::DoneState;
        }
    }
    return false;
}

void TransactionListener::workerEvent(QApt::WorkerEvent event, Transaction *transaction)
{
    if (!transaction || !m_appBackend->transactions().contains(transaction) ||
        m_app != transaction->application()) {
        return;
    }

    switch (event) {
    case QApt::PackageDownloadStarted:
        m_comment = i18nc("@info:status", "Downloading");
        m_progress = 0;
        connect(m_appBackend, SIGNAL(progress(Transaction*,int)),
                this, SLOT(updateProgress(Transaction*,int)));
        emit installing();
        emit commentChanged();
        emit progressChanged();
        break;
    case QApt::PackageDownloadFinished:
        disconnect(m_appBackend, SIGNAL(progress(Transaction*,int)),
                   this, SLOT(updateProgress(Transaction*,int)));
        break;
    case QApt::CommitChangesStarted:
        setStateComment(transaction);
        connect(m_appBackend, SIGNAL(progress(Transaction*,int)),
                this, SLOT(updateProgress(Transaction*,int)));
        break;
    case QApt::CommitChangesFinished:
        emit installing();
        disconnect(m_appBackend, SIGNAL(progress(Transaction*,int)),
                   this, SLOT(updateProgress(Transaction*,int)));
        break;
    default:
        break;
    }
}

void TransactionListener::updateProgress(Transaction *transaction, int percentage)
{
    if (m_app == transaction->application()) {
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
            emit installing();
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

void TransactionListener::transactionCancelled(Application* )
{
    emit installing();
}

void TransactionListener::setApplication(Application* app)
{
    if(m_app) {
        disconnect(m_app, SIGNAL(installChanged()), this, SIGNAL(installing()));
    }
    
    m_app = app;
    emit applicationChanged();
    connect(m_app, SIGNAL(installChanged()), this, SIGNAL(installing()));
}

Application* TransactionListener::application() const
{
    return m_app;
}

ApplicationBackend* TransactionListener::backend() const
{
    return m_appBackend;
}

