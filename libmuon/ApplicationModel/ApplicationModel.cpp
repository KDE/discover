/***************************************************************************
 *   Copyright © 2010 Jonathan Thomas <echidnaman@kubuntu.org>             *
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

#include "ApplicationModel.h"

#include <KIcon>
#include <KLocale>
#include <KExtendableItemDelegate>

#include <LibQApt/Package>

#include <math.h>

#include "Application.h"
#include "ApplicationBackend.h"
#include "ReviewsBackend/Rating.h"
#include "ReviewsBackend/ReviewsBackend.h"
#include "Transaction/Transaction.h"
#include <KDebug>

ApplicationModel::ApplicationModel(QObject *parent)
    : QAbstractListModel(parent)
    , m_appBackend(0)
{
    QHash< int, QByteArray > roles = roleNames();
    roles[NameRole] = "name";
    roles[IconRole] = "icon";
    roles[CommentRole] = "comment";
    roles[ActionRole] = "action";
    roles[StatusRole] = "status";
    roles[RatingRole] = "rating";
    roles[RatingPointsRole] = "ratingPoints";
    roles[SortableRatingRole] = "sortableRating";
    roles[ActiveRole] = "active";
    roles[ProgressRole] = "progress";
    roles[ProgressTextRole] = "progressText";
    roles[InstalledRole] = "installed";
    roles[ApplicationRole] = "application";
    roles[UsageCountRole] = "usageCount";
    roles[PopConRole] = "popcon";
    roles[OriginRole] = "origin";
    roles[UntranslatedNameRole] = "untranslatedName";
    roles[CanUpgrade] = "canUpgrade";
    setRoleNames(roles);
}

ApplicationModel::~ApplicationModel()
{
}

void ApplicationModel::setBackend(ApplicationBackend* backend)
{
    if(m_appBackend) {
        disconnect(m_appBackend, SIGNAL(transactionProgressed(Transaction*,int)),
                    this, SLOT(updateTransactionProgress(Transaction*,int)));
        disconnect(m_appBackend, SIGNAL(workerEvent(QApt::WorkerEvent,Transaction*)),
                this, SLOT(workerEvent(QApt::WorkerEvent,Transaction*)));
        disconnect(m_appBackend, SIGNAL(transactionCancelled(Transaction*)),
                this, SLOT(transactionCancelled(Transaction*)));
        disconnect(m_appBackend->reviewsBackend(), SIGNAL(ratingsReady()), this, SLOT(allDataChanged()));
        disconnect(m_appBackend, SIGNAL(reloadStarted()), this, SLOT(reloadStarted()));
        disconnect(m_appBackend, SIGNAL(reloadFinished()), this, SLOT(reloadFinished()));
        disconnect(m_appBackend, SIGNAL(backendReady()), this, SLOT(reloadApplications()));
    }

    m_appBackend = backend;
    reloadApplications();

    connect(m_appBackend, SIGNAL(transactionProgressed(Transaction*,int)),
                this, SLOT(updateTransactionProgress(Transaction*,int)));
    connect(m_appBackend, SIGNAL(workerEvent(QApt::WorkerEvent,Transaction*)),
            this, SLOT(workerEvent(QApt::WorkerEvent,Transaction*)));
    connect(m_appBackend, SIGNAL(transactionCancelled(Transaction*)),
            this, SLOT(transactionCancelled(Transaction*)));
    connect(m_appBackend->reviewsBackend(), SIGNAL(ratingsReady()), SLOT(allDataChanged()));
    connect(m_appBackend, SIGNAL(reloadStarted()), this, SLOT(reloadStarted()));
    connect(m_appBackend, SIGNAL(reloadFinished()), this, SLOT(reloadFinished()));
    connect(m_appBackend, SIGNAL(backendReady()), this, SLOT(reloadApplications()));
}

void ApplicationModel::reloadStarted()
{
    clear();
}

void ApplicationModel::reloadFinished()
{
    reloadApplications();
}

ApplicationBackend* ApplicationModel::backend() const
{
    return m_appBackend;
}

int ApplicationModel::rowCount(const QModelIndex & parent) const
{
    if(parent.isValid())
        return 0;
    return m_apps.size();
}

QVariant ApplicationModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || m_appBackend->isReloading()) {
        return QVariant();
    }
    switch (role) {
        case NameRole:
            return m_apps.at(index.row())->name();
        case IconRole:
            return m_apps.at(index.row())->icon();
        case CommentRole:
            return m_apps.at(index.row())->comment();
        case StatusRole:
            return m_apps.at(index.row())->package()->state();
        case ActionRole: {
            Transaction *transaction = transactionAt(index);

            if (!transaction) {
                return 0;
            }

            return transaction->action();
        }
        case RatingRole: {
            Rating *rating = m_appBackend->reviewsBackend()->ratingForApplication(m_apps.at(index.row()));
            return rating ? rating->rating() : -1;
        }
        case RatingPointsRole: {
            Rating *rating = m_appBackend->reviewsBackend()->ratingForApplication(m_apps.at(index.row()));
            return rating ? rating->ratingPoints() : -1;
        }
        case SortableRatingRole: {
            Rating *rating = m_appBackend->reviewsBackend()->ratingForApplication(m_apps.at(index.row()));
            return rating ? rating->sortableRating() : -1;
        }
        case ActiveRole: {
            Transaction *transaction = transactionAt(index);

            if (!transaction) {
                return 0;
            }

            if (transaction->state() == (DoneState | InvalidState)) {
                return false;
            }
            return true;
        }
        case ProgressRole: {
            Transaction *transaction = transactionAt(index);

            if (!transaction) {
                return 0;
            }

            if (transaction->state() == RunningState) {
                return m_runningTransactions.value(transaction);
            }
            return 0;
        }
        case ProgressTextRole: {
            Transaction *transaction = transactionAt(index);

            if (!transaction) {
                return QVariant();
            }

            switch(transaction->state()) {
            case QueuedState:
                return i18nc("@info:status Progress text when waiting", "Waiting");
            case DoneState:
                return i18nc("@info:status Progress text when done", "Done");
            case RunningState:
            default:
                break;
            }

            switch (m_appBackend->workerState().first) {
            case QApt::PackageDownloadStarted:
                return i18nc("@info:status", "Downloading");
            case QApt::CommitChangesStarted:
                switch (index.data(ApplicationModel::ActionRole).toInt()) {
                case InstallApp:
                    return i18nc("@info:status", "Installing");
                case ChangeAddons:
                    return i18nc("@info:status", "Changing Addons");
                case RemoveApp:
                    return i18nc("@info:status", "Removing");
                default:
                    return QVariant();
                }
            default:
                return QVariant();
            }
        }
        break;
        case InstalledRole:
            return m_apps.at(index.row())->isInstalled();
        case Qt::ToolTipRole:
            return QVariant();
        case ApplicationRole:
            return qVariantFromValue<QObject*>(m_apps.at(index.row()));
        case UsageCountRole:
            return m_apps.at(index.row())->usageCount();
        case PopConRole:
            return m_apps.at(index.row())->popularityContest();
        case OriginRole:
            return m_apps.at(index.row())->origin();
        case UntranslatedNameRole:
            return m_apps.at(index.row())->untranslatedName();
        case CanUpgrade:
            return m_apps.at(index.row())->canUpgrade();
    }

    return QVariant();
}

void ApplicationModel::clear()
{
    beginRemoveRows(QModelIndex(), 0, m_apps.count()-1);
    m_apps.clear();
    m_runningTransactions.clear();
    endRemoveRows();
}

void ApplicationModel::updateTransactionProgress(Transaction *trans, int progress)
{
    if (!m_appBackend->transactions().contains(trans)) {
        return;
    }
    m_runningTransactions[trans] = progress;

    emit dataChanged(index(m_apps.indexOf(qobject_cast<Application*>(trans->application())), 0),
                         index(m_apps.indexOf(qobject_cast<Application*>(trans->application())), 0));
}

void ApplicationModel::workerEvent(QApt::WorkerEvent event, Transaction *trans)
{
    Q_UNUSED(event);

    if (!m_appBackend->transactions().contains(trans)) {
        return;
    }

    if (trans != 0) {
        int app = m_apps.indexOf(qobject_cast<Application*>(trans->application()));
        emit dataChanged(index(app, 0),
                         index(app, 0));
    }
}

void ApplicationModel::transactionCancelled(Transaction *trans)
{
    QModelIndex idx = index(m_apps.indexOf(qobject_cast<Application*>(trans->application())), 0);
    emit dataChanged(idx, idx);
}

Application *ApplicationModel::applicationAt(const QModelIndex &index) const
{
    return m_apps.at(index.row());
}

Transaction *ApplicationModel::transactionAt(const QModelIndex &index) const
{
    Transaction *transaction = 0;

    Application *app = applicationAt(index);
    foreach (Transaction *trns, m_appBackend->transactions()) {
        if (trns->application() == app) {
            transaction = trns;
        }
    }

    return transaction;
}

void ApplicationModel::reloadApplications()
{
    beginResetModel();
    m_apps = m_appBackend->applicationList();
    m_runningTransactions.clear();
    endResetModel();
}

void ApplicationModel::allDataChanged()
{
    emit dataChanged(index(0,0), index(rowCount(), 0));
}

#include "ApplicationModel.moc"
