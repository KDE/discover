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

#include "../Application.h"
#include "../ApplicationBackend.h"
#include "../Transaction.h"

ApplicationModel::ApplicationModel(QObject *parent, ApplicationBackend *backend)
    : QAbstractListModel(parent)
    , m_appBackend(backend)
    , m_apps()
    , m_maxPopcon(0)
{
    connect(m_appBackend, SIGNAL(progress(Transaction *, int)),
                this, SLOT(updateTransactionProgress(Transaction *, int)));
    connect(m_appBackend, SIGNAL(workerEvent(QApt::WorkerEvent, Transaction *)),
            this, SLOT(workerEvent(QApt::WorkerEvent, Transaction *)));
}

ApplicationModel::~ApplicationModel()
{
}

int ApplicationModel::rowCount(const QModelIndex & /*parent*/) const
{
    return m_apps.size();
}

int ApplicationModel::columnCount(const QModelIndex & /*parent*/) const
{
    return 1;
}

QVariant ApplicationModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) {
        return false;
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
        case PopconRole:
            // Take the log of the popcon score, divide by max +1, then multiply by number
            // of star steps. (10 in the case of KRatingsPainter)
            return (int)(10* log(m_apps.at(index.row())->popconScore())/log(m_maxPopcon+1));
        case ActiveRole: {
            Transaction *transaction = transactionAt(index);

            if (!transaction) {
                return 0;
            }

            if (transaction->state() != InvalidState) {
                return true;
            }
            return false;
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
                return i18nc("@info:status", "Waiting");
            case DoneState:
                return i18nc("@info:status", "Done");
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
        case Qt::ToolTipRole:
            return QVariant();
    }

    return QVariant();
}

void ApplicationModel::setApplications(const QList<Application*> &list)
{
    // Remove check when sid has >= 4.7
#if QT_VERSION >= 0x040700
    m_apps.reserve(list.size());
#endif
    beginInsertRows(QModelIndex(), m_apps.count(), m_apps.count());
    m_apps = list;
    endInsertRows();
}

void ApplicationModel::setMaxPopcon(int popconScore)
{
    m_maxPopcon = popconScore;
}

void ApplicationModel::clear()
{
    beginRemoveRows(QModelIndex(), 0, m_apps.size());
    m_apps.clear();
    m_runningTransactions.clear();
    m_maxPopcon = 0;
    endRemoveRows();
}

void ApplicationModel::updateTransactionProgress(Transaction *trans, int progress)
{
    m_runningTransactions[trans] = progress;

    emit dataChanged(index(m_apps.indexOf(trans->application()), 0),
                         index(m_apps.indexOf(trans->application()), 0));
}

void ApplicationModel::workerEvent(QApt::WorkerEvent event, Transaction *trans)
{
    Q_UNUSED(event);

    if (trans != 0) {
        emit dataChanged(index(m_apps.indexOf(trans->application()), 0),
                         index(m_apps.indexOf(trans->application()), 0));
    }
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

QList<Application*> ApplicationModel::applications() const
{
    return m_apps;
}

#include "ApplicationModel.moc"
