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

#include "ResourcesUpdatesModel.h"
#include <Transaction/Transaction.h>
#include <Transaction/TransactionModel.h>
#include "ResourcesModel.h"
#include "AbstractBackendUpdater.h"
#include "AbstractResource.h"
#include <QDateTime>
#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusMessage>
#include <QDBusReply>
#include <QDebug>

#include <KLocalizedString>
#include <KFormat>

class UpdateTransaction : public Transaction
{
    Q_OBJECT
public:
    UpdateTransaction(ResourcesUpdatesModel* parent)
        : Transaction(nullptr, nullptr, Transaction::InstallRole)
        , m_allUpdaters(parent->updaters())
    {
        bool cancelable = false;
        foreach(auto updater, parent->updaters()) {
            connect(updater, &AbstractBackendUpdater::progressingChanged, this, &UpdateTransaction::slotProgressingChanged);
            connect(updater, &AbstractBackendUpdater::progressChanged, this, &UpdateTransaction::slotUpdateProgress);
            connect(updater, &AbstractBackendUpdater::passiveMessage, this, &Transaction::passiveMessage);
            connect(updater, &AbstractBackendUpdater::proceedRequest, this, &UpdateTransaction::processProceedRequest);
            cancelable |= updater->isCancelable();
        }
        setCancellable(cancelable);
    }

    void processProceedRequest(const QString &title, const QString& message) {
        m_updatersWaitingForFeedback += qobject_cast<AbstractBackendUpdater*>(sender());
        Q_EMIT proceedRequest(title, message);
    }

    void cancel() override {
        QVector<AbstractBackendUpdater*> toCancel = m_updatersWaitingForFeedback.isEmpty() ? m_allUpdaters : m_updatersWaitingForFeedback;

        foreach(auto updater, m_updatersWaitingForFeedback) {
            updater->cancel();
        }
    }

    void proceed() override {
        m_updatersWaitingForFeedback.takeFirst()->proceed();
    }

    bool isProgressing() const
    {
        bool progressing = false;
        foreach(AbstractBackendUpdater* upd, m_allUpdaters) {
            progressing |= upd->isProgressing();
        }
        return progressing;
    }

    void slotProgressingChanged()
    {
        if (status() < DoneStatus && !isProgressing()) {
            setStatus(Transaction::DoneStatus);
            Q_EMIT finished();
            qDebug() << "finished!" << this << sender();
            deleteLater();
        }
    }

    void slotUpdateProgress()
    {
        qreal total = 0;
        foreach(AbstractBackendUpdater* updater, m_allUpdaters) {
            total += updater->progress();
        }
        setProgress(total / m_allUpdaters.count());
    }

    QVariant icon() const override { return QStringLiteral("update-low"); }
    QString name() const override { return i18n("Update"); }

Q_SIGNALS:
    void finished();

private:
    QVector<AbstractBackendUpdater*> m_updatersWaitingForFeedback;
    const QVector<AbstractBackendUpdater*> m_allUpdaters;
};

ResourcesUpdatesModel::ResourcesUpdatesModel(QObject* parent)
    : QStandardItemModel(parent)
    , m_lastIsProgressing(false)
    , m_transaction(nullptr)
{
    connect(ResourcesModel::global(), &ResourcesModel::backendsChanged, this, &ResourcesUpdatesModel::init);

    init();
}

void ResourcesUpdatesModel::init()
{
    const QVector<AbstractResourcesBackend*> backends = ResourcesModel::global()->backends();
    m_lastIsProgressing = false;
    foreach(AbstractResourcesBackend* b, backends) {
        AbstractBackendUpdater* updater = b->backendUpdater();
        if(updater && !m_updaters.contains(updater)) {
            connect(updater, &AbstractBackendUpdater::statusMessageChanged, this, &ResourcesUpdatesModel::message);
            connect(updater, &AbstractBackendUpdater::statusDetailChanged, this, &ResourcesUpdatesModel::message);
            connect(updater, &AbstractBackendUpdater::remainingTimeChanged, this, &ResourcesUpdatesModel::etaChanged);
            connect(updater, &AbstractBackendUpdater::downloadSpeedChanged, this, &ResourcesUpdatesModel::downloadSpeedChanged);
            connect(updater, &AbstractBackendUpdater::resourceProgressed, this, &ResourcesUpdatesModel::resourceProgressed);
            connect(updater, &AbstractBackendUpdater::destroyed, this, &ResourcesUpdatesModel::updaterDestroyed);
            m_updaters += updater;

            m_lastIsProgressing |= updater->isProgressing();
        }
    }

    auto tm = TransactionModel::global();
    foreach(auto t, tm->transactions()) {
        auto updateTransaction = qobject_cast<UpdateTransaction*>(t);
        if (updateTransaction) {
            setTransaction(updateTransaction);
        }
    }
}

void ResourcesUpdatesModel::updaterDestroyed(QObject* obj)
{
    m_updaters.removeAll(static_cast<AbstractBackendUpdater*>(obj));
}

void ResourcesUpdatesModel::message(const QString& msg)
{
    if(msg.isEmpty())
        return;

    appendRow(new QStandardItem(msg));
}

void ResourcesUpdatesModel::prepare()
{
    if(isProgressing()) {
        qWarning() << "trying to set up a running instance";
        return;
    }
    foreach(AbstractBackendUpdater* upd, m_updaters) {
        upd->prepare();
    }
}

void ResourcesUpdatesModel::updateAll()
{
    if (!m_updaters.isEmpty()) {
        delete m_transaction;
        m_transaction = new UpdateTransaction(this);
        setTransaction(m_transaction);
        TransactionModel::global()->addTransaction(m_transaction);
        Q_FOREACH (AbstractBackendUpdater* upd, m_updaters) {
            if (upd->hasUpdates())
                QMetaObject::invokeMethod(upd, "start", Qt::QueuedConnection);
        }
    }
}


QString ResourcesUpdatesModel::remainingTime() const
{
    long unsigned int maxEta = 0;
    foreach(AbstractBackendUpdater* upd, m_updaters) {
        maxEta = qMax(maxEta, upd->remainingTime());
    }

    // Ignore ETA if it's larger than 2 days.
    if(maxEta > 2 * 24 * 60 * 60)
        return QString();
    else if(maxEta==0)
        return i18nc("@item:intext Unknown remaining time", "Updating...");
    else
        return i18nc("@item:intext Remaining time", "%1 remaining", KFormat().formatDuration(maxEta));
}

bool ResourcesUpdatesModel::isProgressing() const
{
    return m_transaction && m_transaction->status() < Transaction::DoneStatus;
}

QList<AbstractResource*> ResourcesUpdatesModel::toUpdate() const
{
    QList<AbstractResource*> ret;
    foreach(AbstractBackendUpdater* upd, m_updaters) {
        ret += upd->toUpdate();
    }
    return ret;
}

void ResourcesUpdatesModel::addResources(const QList<AbstractResource*>& resources)
{
    QHash<AbstractResourcesBackend*, QList<AbstractResource*> > sortedResources;
    foreach(AbstractResource* res, resources) {
        sortedResources[res->backend()] += res;
    }

    for(auto it=sortedResources.constBegin(), itEnd=sortedResources.constEnd(); it!=itEnd; ++it) {
        it.key()->backendUpdater()->addResources(*it);
    }
}

void ResourcesUpdatesModel::removeResources(const QList< AbstractResource* >& resources)
{
    QHash<AbstractResourcesBackend*, QList<AbstractResource*> > sortedResources;
    foreach(AbstractResource* res, resources) {
        sortedResources[res->backend()] += res;
    }

    for(auto it=sortedResources.constBegin(), itEnd=sortedResources.constEnd(); it!=itEnd; ++it) {
        it.key()->backendUpdater()->removeResources(*it);
    }
}

QDateTime ResourcesUpdatesModel::lastUpdate() const
{
    QDateTime ret;
    foreach(AbstractBackendUpdater* upd, m_updaters) {
        QDateTime current = upd->lastUpdate();
        if(!ret.isValid() || (current.isValid() && current>ret)) {
            ret = current;
        }
    }
    return ret;
}

qint64 ResourcesUpdatesModel::secsToLastUpdate() const
{
    return lastUpdate().secsTo(QDateTime::currentDateTime());
}

void ResourcesUpdatesModel::setTransaction(UpdateTransaction* transaction)
{
    m_transaction = transaction;
    connect(m_transaction, &UpdateTransaction::finished, this, &ResourcesUpdatesModel::finished);
    connect(m_transaction, &UpdateTransaction::finished, this, &ResourcesUpdatesModel::progressingChanged);

    Q_EMIT progressingChanged();
}

#include "ResourcesUpdatesModel.moc"
