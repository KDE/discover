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

// KDE includes
#include <KLocale>

TransactionModel::TransactionModel(QObject *parent)
    : QAbstractListModel(parent)
{
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

    Transaction2 *trans = transactionFromIndex(index);
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
        case SetupStatus:
            return i18nc("@info:status", "Starting");
        case QueuedStatus:
            return i18nc("@info:status", "Waiting");
        case DownloadingStatus:
            return i18nc("@info:status", "Downloading");
        case CommittingStatus:
            switch (trans->role()) {
            case InstallRole:
                return i18nc("@info:status", "Installing");
            case RemoveRole:
                return i18nc("@info:status", "Removing");
            case ChangeAddonsRole:
                return i18nc("@info:status", "Changing Addons");
            }
            break;
        }
        break;
    }

    return QVariant();
}

Transaction2 *TransactionModel::transactionFromIndex(const QModelIndex &index) const
{
    if (index.isValid())
        return static_cast<Transaction2 *>(index.internalPointer());

    return nullptr;
}

void TransactionModel::addTransaction(Transaction2 *trans)
{
    if (m_transactions.contains(trans))
        return;

    // Connect all notify signals to our transactionChanged slot
    const QMetaObject *meta = trans->metaObject();
    for (int i = 0; i < meta->propertyCount(); ++i) {
        QMetaProperty prop = meta->property(i);

        if (prop.notifySignalIndex() == -1)
            continue;

        const QMetaMethod notifySignal = prop.notifySignal();
        const QMetaMethod notifySlot = metaObject()->method(metaObject()->indexOfSlot("transactionChanged()"));
        connect(trans, notifySignal, this, notifySlot);
    }
}

void TransactionModel::transactionChanged()
{
    // FIXME: determine which trans changed via sender(), find index, give proper bounds to dataChanged
    emit dataChanged(QModelIndex(), QModelIndex());
}
