/*
 *   Copyright (C) 2012 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library/Lesser General Public License
 *   version 2, or (at your option) any later version, as published by the
 *   Free Software Foundation
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details
 *
 *   You should have received a copy of the GNU Library/Lesser General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <resources/StandardBackendUpdater.h>
#include <resources/AbstractResourcesBackend.h>
#include <resources/AbstractResource.h>
#include "ResourcesModel.h"
#include <Transaction/Transaction.h>
#include <Transaction/TransactionModel.h>
#include <KLocalizedString>
#include <QDateTime>
#include "libdiscover_debug.h"
#include "utils.h"
#include <QTimer>
#include <QIcon>

StandardBackendUpdater::StandardBackendUpdater(AbstractResourcesBackend* parent)
    : AbstractBackendUpdater(parent)
    , m_backend(parent)
    , m_settingUp(false)
    , m_progress(0)
    , m_lastUpdate(QDateTime())
{
    connect(m_backend, &AbstractResourcesBackend::fetchingChanged, this, &StandardBackendUpdater::refreshUpdateable);
    connect(m_backend, &AbstractResourcesBackend::resourcesChanged, this, &StandardBackendUpdater::resourcesChanged);
    connect(m_backend, &AbstractResourcesBackend::resourceRemoved, this, [this](AbstractResource* resource){
        m_upgradeable.remove(resource);
        m_toUpgrade.remove(resource);
    });
    connect(TransactionModel::global(), &TransactionModel::transactionRemoved, this, &StandardBackendUpdater::transactionRemoved);
    connect(TransactionModel::global(), &TransactionModel::transactionAdded, this, &StandardBackendUpdater::transactionAdded);

    m_timer.setSingleShot(true);
    m_timer.setInterval(10);
    connect(&m_timer, &QTimer::timeout, this, &StandardBackendUpdater::refreshUpdateable);
}

void StandardBackendUpdater::resourcesChanged(AbstractResource* res, const QVector<QByteArray>& props)
{
    if (props.contains("state") && (res->state() == AbstractResource::Upgradeable || m_upgradeable.contains(res)))
        m_timer.start();
}

bool StandardBackendUpdater::hasUpdates() const
{
    return !m_upgradeable.isEmpty();
}

void StandardBackendUpdater::start()
{
    m_settingUp = true;
    emit progressingChanged(true);
    setProgress(0);
    auto upgradeList = m_toUpgrade.values();
    std::sort(upgradeList.begin(), upgradeList.end(), [](const AbstractResource* a, const AbstractResource* b){ return a->name() < b->name(); });

    const bool couldCancel = m_canCancel;
    foreach(AbstractResource* res, upgradeList) {
        m_pendingResources += res;
        auto t = m_backend->installApplication(res);
        t->setVisible(false);
        t->setProperty("updater", QVariant::fromValue<QObject*>(this));
        connect(t, &Transaction::downloadSpeedChanged, this, [this](){
            Q_EMIT downloadSpeedChanged(downloadSpeed());
        });
        connect(this, &StandardBackendUpdater::cancelTransaction, t, &Transaction::cancel);
        TransactionModel::global()->addTransaction(t);
        m_canCancel |= t->isCancellable();
    }
    if (m_canCancel != couldCancel) {
        Q_EMIT cancelableChanged(m_canCancel);
    }
    m_settingUp = false;

    if(m_pendingResources.isEmpty()) {
        cleanup();
    } else {
        setProgress(1);
    }
}

void StandardBackendUpdater::cancel()
{
    Q_EMIT cancelTransaction();
}

void StandardBackendUpdater::transactionAdded(Transaction* newTransaction)
{
    if (!m_pendingResources.contains(newTransaction->resource()))
        return;

    connect(newTransaction, &Transaction::progressChanged, this, &StandardBackendUpdater::transactionProgressChanged);
    connect(newTransaction, &Transaction::statusChanged, this, &StandardBackendUpdater::transactionProgressChanged);
}

AbstractBackendUpdater::State toUpdateState(Transaction* t)
{
    switch(t->status()) {
        case Transaction::SetupStatus:
        case Transaction::QueuedStatus:
            return AbstractBackendUpdater::None;
        case Transaction::DownloadingStatus:
            return AbstractBackendUpdater::Downloading;
        case Transaction::CommittingStatus:
            return AbstractBackendUpdater::Installing;
        case Transaction::DoneStatus:
        case Transaction::DoneWithErrorStatus:
        case Transaction::CancelledStatus:
            return AbstractBackendUpdater::Done;
    }
    Q_UNREACHABLE();
}

void StandardBackendUpdater::transactionProgressChanged()
{
    Transaction* t = qobject_cast<Transaction*>(sender());
    Q_EMIT resourceProgressed(t->resource(), t->progress(), toUpdateState(t));

    refreshProgress();
}

void StandardBackendUpdater::transactionRemoved(Transaction* t)
{
    const bool fromOurBackend = t->resource() && t->resource()->backend()==m_backend;
    if (!fromOurBackend) {
        return;
    }

    const bool found = fromOurBackend && m_pendingResources.remove(t->resource());

    if(found && !m_settingUp) {
        refreshProgress();
        if(m_pendingResources.isEmpty()) {
            cleanup();
        }
    }
    refreshUpdateable();
}

void StandardBackendUpdater::refreshProgress()
{
    if (m_toUpgrade.isEmpty()) {
        return;
    }

    int allProgresses = (m_toUpgrade.size() - m_pendingResources.size()) * 100;
    const auto allTransactions = transactions();
    for (auto t: allTransactions) {
        allProgresses += t->progress();
    }
    setProgress(allProgresses / m_toUpgrade.size());
}

void StandardBackendUpdater::refreshUpdateable()
{
    if (m_backend->isFetching() || !m_backend->isValid()) {
        return;
    }

    if (isProgressing()) {
        m_timer.start(1000);
        return;
    }

    m_settingUp = true;
    Q_EMIT progressingChanged(true);
    AbstractResourcesBackend::Filters f;
    f.state = AbstractResource::Upgradeable;
    m_upgradeable.clear();
    auto r = m_backend->search(f);
    connect(r, &ResultsStream::resourcesFound, this, [this](const QVector<AbstractResource*> &resources){
        for(auto res : resources)
            if (res->state() == AbstractResource::Upgradeable)
                m_upgradeable.insert(res);
    });
    connect(r, &ResultsStream::destroyed, this, [this](){
        m_settingUp = false;
        Q_EMIT updatesCountChanged(updatesCount());
        Q_EMIT progressingChanged(false);
    });
}

qreal StandardBackendUpdater::progress() const
{
    return m_progress;
}

void StandardBackendUpdater::setProgress(qreal p)
{
    if(p>m_progress || p<0) {
        m_progress = p;
        emit progressChanged(p);
    }
}

void StandardBackendUpdater::prepare()
{
    m_lastUpdate = QDateTime::currentDateTime();
    m_toUpgrade = m_upgradeable;
}

int StandardBackendUpdater::updatesCount() const
{
    return m_upgradeable.count();
}

void StandardBackendUpdater::addResources(const QList< AbstractResource* >& apps)
{
    const QSet<AbstractResource *> upgradeableApps = kToSet(apps);
    Q_ASSERT(m_upgradeable.contains(upgradeableApps));
    m_toUpgrade += upgradeableApps;
}

void StandardBackendUpdater::removeResources(const QList< AbstractResource* >& apps)
{
    const QSet<AbstractResource *> upgradeableApps = kToSet(apps);
    Q_ASSERT(m_upgradeable.contains(upgradeableApps));
    Q_ASSERT(m_toUpgrade.contains(upgradeableApps));
    m_toUpgrade -= upgradeableApps;
}

void StandardBackendUpdater::cleanup()
{
    m_lastUpdate = QDateTime::currentDateTime();
    m_toUpgrade.clear();

    refreshUpdateable();
    emit progressingChanged(false);
}

QList<AbstractResource*> StandardBackendUpdater::toUpdate() const
{
    return m_toUpgrade.values();
}

bool StandardBackendUpdater::isMarked(AbstractResource* res) const
{
    return m_toUpgrade.contains(res);
}

QDateTime StandardBackendUpdater::lastUpdate() const
{
    return m_lastUpdate;
}

bool StandardBackendUpdater::isCancelable() const
{
    return m_canCancel;
}

bool StandardBackendUpdater::isProgressing() const
{
    return m_settingUp || !m_pendingResources.isEmpty();
}

double StandardBackendUpdater::updateSize() const
{
    double ret = 0.;
    for(AbstractResource* res: m_toUpgrade) {
        ret += res->size();
    }
    return ret;
}

QVector<Transaction *> StandardBackendUpdater::transactions() const
{
    const auto trans = TransactionModel::global()->transactions();
    return kFilter<QVector<Transaction*>>(trans, [this](Transaction* t) { return t->property("updater").value<QObject*>() == this; });
}

quint64 StandardBackendUpdater::downloadSpeed() const
{
    quint64 ret = 0;
    const auto trans = transactions();
    for(Transaction* t: trans) {
        ret += t->downloadSpeed();
    }
    return ret;

}
