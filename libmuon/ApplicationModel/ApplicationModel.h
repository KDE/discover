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

#ifndef APPLICATIONMODEL_H
#define APPLICATIONMODEL_H

#include <QAbstractListModel>
#include <QVector>

#include <LibQApt/Globals>

#include "libmuonprivate_export.h"

class Application;
class ApplicationBackend;
class Transaction;

class MUONPRIVATE_EXPORT ApplicationModel: public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(ApplicationBackend* backend READ backend WRITE setBackend)
    Q_ENUMS(Roles)
public:
    enum Roles {
        NameRole = Qt::UserRole,
        IconRole,
        CommentRole,
        ActionRole,
        StatusRole,
        RatingRole,
        RatingPointsRole,
        ActiveRole,
        ProgressRole,
        ProgressTextRole,
        InstalledRole,
        ApplicationRole,
        UsageCountRole,
        PopConRole,
        UntranslatedNameRole,
        OriginRole
    };
    explicit ApplicationModel(QObject* parent=0);
    ~ApplicationModel();

    void setBackend(ApplicationBackend* backend);
    ApplicationBackend* backend() const;
    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;

    void clear();
    Application *applicationAt(const QModelIndex &index) const;
    Transaction *transactionAt(const QModelIndex &index) const;
    QVector<Application *> applications() const;
    
private:
    ApplicationBackend *m_appBackend;
    QVector<Application *> m_apps;
    QHash<Transaction *, int> m_runningTransactions;

public Q_SLOTS:
    void updateTransactionProgress(Transaction *transaction, int progress);
    void allDataChanged();
    void reloadApplications();

private Q_SLOTS:
    void workerEvent(QApt::WorkerEvent event, Transaction *trans);
    void transactionCancelled(Application *app);
    void reloadStarted();
    void reloadFinished();

Q_SIGNALS:
   void dataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight);
};

#endif
