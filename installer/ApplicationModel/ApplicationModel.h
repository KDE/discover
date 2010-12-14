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

#include <LibQApt/Globals>

class Application;
class ApplicationBackend;
class Transaction;

class ApplicationModel: public QAbstractListModel
{
    Q_OBJECT
public:
    enum {
        NameRole = Qt::UserRole,
        IconRole = Qt::UserRole + 1,
        CommentRole = Qt::UserRole + 2,
        ActionRole = Qt::UserRole + 3,
        StatusRole = Qt::UserRole + 4,
        PopconRole = Qt::UserRole + 5,
        ActiveRole = Qt::UserRole + 6,
        ProgressRole = Qt::UserRole + 7,
        ProgressTextRole = Qt::UserRole + 8
    };
    explicit ApplicationModel(QObject *parent, ApplicationBackend *backend);
    ~ApplicationModel();

    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;

    void setApplications(const QList<Application*> &list);
    void setMaxPopcon(int popconScore);
    void clear();
    Application *applicationAt(const QModelIndex &index) const;
    Transaction *transactionAt(const QModelIndex &index) const;
    QList<Application *> applications() const;

private:
    ApplicationBackend *m_appBackend;
    QList<Application *> m_apps;
    QHash<Transaction *, int> m_runningTransactions;
    int m_maxPopcon;

public Q_SLOTS:
    void updateTransactionProgress(Transaction *transaction, int progress);

private Q_SLOTS:
    void workerEvent(QApt::WorkerEvent event, Transaction *trans);

Q_SIGNALS:
   void dataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight);
};

#endif
