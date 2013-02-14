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

#ifndef ABSTRACTRESOURCESBACKEND_H
#define ABSTRACTRESOURCESBACKEND_H

#include <QObject>
#include <QPair>
#include <QVector>

#include "libmuonprivate_export.h"

enum TransactionStateTransition {
    StartedDownloading,
    FinishedDownloading,
    StartedCommitting,
    FinishedCommitting
};

class Transaction;
class AbstractReviewsBackend;
class AbstractResource;
class AbstractBackendUpdater;
class MuonMainWindow;

class MUONPRIVATE_EXPORT AbstractResourcesBackend : public QObject
{
    Q_OBJECT
    Q_PROPERTY(AbstractReviewsBackend* reviewsBackend READ reviewsBackend CONSTANT)
    Q_PROPERTY(int updatesCount READ updatesCount NOTIFY updatesCountChanged)
    public:
        explicit AbstractResourcesBackend(QObject* parent = 0);
        virtual bool isValid() const = 0;
        virtual QVector<AbstractResource*> allResources() const = 0;
        virtual QStringList searchPackageName(const QString &searchText) = 0;
        virtual AbstractReviewsBackend* reviewsBackend() const = 0;
        virtual AbstractBackendUpdater* backendUpdater() const = 0;
        virtual int updatesCount() const = 0;
        virtual QPair<TransactionStateTransition, Transaction *> currentTransactionState() const = 0;
        virtual QList<Transaction*> transactions() const = 0;
        virtual AbstractResource* resourceByPackageName(const QString& name) const = 0;
        virtual QList<AbstractResource*> upgradeablePackages() const = 0;
        virtual void integrateMainWindow(MuonMainWindow* w);

    public slots:
        virtual void installApplication(AbstractResource *app, const QHash<QString, bool> &addons) = 0;
        virtual void installApplication(AbstractResource *app);
        virtual void removeApplication(AbstractResource *app) = 0;
        virtual void cancelTransaction(AbstractResource *app) = 0;

    signals:
        void backendReady();
        void reloadStarted();
        void reloadFinished();
        void updatesCountChanged();
        void allDataChanged();
        void searchInvalidated();
        
        void transactionProgressed(Transaction *transaction, int progress);
        void transactionAdded(Transaction *transaction);
        void transactionCancelled(Transaction *app);
        void transactionRemoved(Transaction* t);
        void transactionsEvent(TransactionStateTransition transition, Transaction* transaction);
};

Q_DECLARE_INTERFACE( AbstractResourcesBackend, "org.kde.muon.AbstractResourcesBackend" )

#endif // ABSTRACTRESOURCESBACKEND_H
