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

#include "LaunchListModel.h"

// Qt includes
#include <QStringBuilder>

// KDE includes
#include <KService>
#include <KToolInvocation>

// Libmuon includes
#include "Transaction/TransactionModel.h"
#include "resources/AbstractResource.h"

LaunchListModel::LaunchListModel(QObject* parent)
    : QStandardItemModel(parent)
{
    connect(TransactionModel::global(), SIGNAL(transactionAdded(Transaction*)),
            this, SLOT(watchTransaction(Transaction*)));
}

void LaunchListModel::watchTransaction(Transaction *trans)
{
    connect(trans, SIGNAL(statusChanged(Transaction::Status)),
            this, SLOT(transactionStatusChanged(Transaction::Status)));
}

void LaunchListModel::transactionStatusChanged(Transaction::Status status)
{
    Transaction *trans = qobject_cast<Transaction *>(sender());

    if (status == Transaction::DoneStatus)
        transactionFinished(trans);
}

void LaunchListModel::transactionFinished(Transaction* trans)
{
    bool doneInstall = trans->status() == Transaction::DoneStatus &&
                       trans->role() == Transaction::InstallRole;

    if(trans->resource()->canExecute() && doneInstall)
        addApplication(trans->resource());
}

void LaunchListModel::addApplication(AbstractResource* app)
{
    QList<QStandardItem*> items;
    QStringList execs = app->executables();

    for (const QString& exec : execs) {
        KService::Ptr service = KService::serviceByStorageId(exec);

        QString name = service->property("Name").toString();
        if (!service->genericName().isEmpty())
            name += QLatin1String(" - ") % service->genericName();

        QStandardItem *item = new QStandardItem(name);
        item->setIcon(QIcon::fromTheme(service->icon()));
        item->setData(service->desktopEntryPath(), Qt::UserRole);
        items += item;
    }
    if(!items.isEmpty())
        invisibleRootItem()->appendRows(items);
}

void LaunchListModel::invokeApplication(const QModelIndex &idx) const
{
    KToolInvocation::startServiceByDesktopPath(idx.data(Qt::UserRole).toString());
}

void LaunchListModel::invokeApplication(int row) const
{
    invokeApplication(index(row, 0));
}
