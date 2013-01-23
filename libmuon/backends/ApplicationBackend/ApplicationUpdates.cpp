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

#include "ApplicationUpdates.h"

// Qt includes
#include <QIcon>

// KDE includes
#include <KProtocolManager>

// LibQApt includes
#include <LibQApt/Backend>
#include <LibQApt/Transaction>

// Own includes
#include "Application.h"
#include "ApplicationBackend.h"
#include "ChangesDialog.h"

ApplicationUpdates::ApplicationUpdates(ApplicationBackend* parent)
    : AbstractBackendUpdater(parent)
    , m_aptBackend(nullptr)
    , m_appBackend(parent)
    , m_lastRealProgress(0)
    , m_eta(0)
{
}

bool ApplicationUpdates::hasUpdates() const
{
    return m_appBackend->updatesCount()>0;
}

qreal ApplicationUpdates::progress() const
{
    return m_lastRealProgress;
}

long unsigned int ApplicationUpdates::remainingTime() const
{
    return m_eta;
}

void ApplicationUpdates::setBackend(QApt::Backend* backend)
{
    Q_ASSERT(!m_aptBackend || m_aptBackend==backend);
    m_aptBackend = backend;
}

void ApplicationUpdates::start()
{
    m_aptBackend->saveCacheState();

    // Take the current cache state to compare for changes later
    QApt::CacheState cache = m_aptBackend->currentCacheState();
    m_aptBackend->markPackagesForDistUpgrade();
    auto changes = m_aptBackend->stateChanges(cache, QApt::PackageList());
    changes.remove(QApt::Package::ToUpgrade);

    // Confirm additional changes beyond upgrading the files
    if(!changes.isEmpty()) {
        ChangesDialog d(m_appBackend->mainWindow(), changes);
        if(d.exec()==QDialog::Rejected) {
            emit updatesFinnished();
            return;
        }
    }

    // Create and run the transaction
    setupTransaction(m_aptBackend->commitChanges());
}

void ApplicationUpdates::progressChanged(int progress)
{
    if (progress > 100)
        return;

    if (progress > m_lastRealProgress) {
        m_lastRealProgress = progress;
        emit progressChanged((qreal)progress);
    }
}

void ApplicationUpdates::etaChanged(quint64 eta)
{
    m_eta = eta;
    emit remainingTimeChanged();
}

void ApplicationUpdates::installMessage(const QString& msg)
{
    emit message(QIcon(), msg);
}

void ApplicationUpdates::transactionStatusChanged(QApt::TransactionStatus status)
{
    if (status == QApt::FinishedStatus) {
        emit updatesFinnished();
        m_lastRealProgress = 0;
    }
}

void ApplicationUpdates::errorOccurred(QApt::ErrorCode error)
{
    emit errorSignal(error, m_trans->errorDetails());
}

void ApplicationUpdates::setupTransaction(QApt::Transaction *trans)
{
    Q_ASSERT(!m_trans);
    // Provide proxy/locale to the transaction
    if (KProtocolManager::proxyType() == KProtocolManager::ManualProxy) {
        trans->setProxy(KProtocolManager::proxyFor("http"));
    }

    trans->setLocale(QLatin1String(setlocale(LC_MESSAGES, 0)));
    m_trans = trans;

    connect(m_trans, SIGNAL(statusChanged(QApt::TransactionStatus)),
            this, SLOT(transactionStatusChanged(QApt::TransactionStatus)));
    connect(m_trans, SIGNAL(errorOccurred(QApt::ErrorCode)),
            this, SLOT(errorOccurred(QApt::ErrorCode)));
    connect(m_trans, SIGNAL(progressChanged(int)),
            this, SLOT(progressChanged(int)));
    connect(m_trans, SIGNAL(statusDetailsChanged(QString)),
            this, SLOT(installMessage(QString)));
    m_trans->run();
}
