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

#include <QtCore/QObject>
#include <QtCore/QPair>
#include <QtCore/QVector>

#include "Transaction/AddonList.h"

#include "libmuonprivate_export.h"

class Transaction;
class AbstractReviewsBackend;
class AbstractResource;
class AbstractBackendUpdater;
class MuonMainWindow;

/**
 * \class AbstractResourcesBackend  AbstractResourcesBackend.h "AbstractResourcesBackend.h"
 *
 * \brief This is the base class of all resource backends.
 * 
 * For writing basic new resource backends, we need to implement two classes: this and the 
 * AbstractResource one. Basic questions on how to build your plugin with those classes
 * can be answered by looking at the dummy plugin.
 * 
 * As this is the base class of a backend, we save all the created resources here and also
 * accept calls to install and remove applications or to cancel transactions.
 * 
 * To show resources in Muon, we need to initialize all resources we want to show beforehand,
 * we should not create resources in the search function. When we reload the resources
 * (e.g. when initializing), the backend needs change the fetching property throughout the
 * processs.
 */
class MUONPRIVATE_EXPORT AbstractResourcesBackend : public QObject
{
    Q_OBJECT
    Q_PROPERTY(AbstractReviewsBackend* reviewsBackend READ reviewsBackend CONSTANT)
    Q_PROPERTY(int updatesCount READ updatesCount NOTIFY updatesCountChanged)
    Q_PROPERTY(bool isFetching READ isFetching NOTIFY fetchingChanged)
    public:
        /**
         * Constructs an AbstractResourcesBackend
         * @param parent the parent of the class (the object will be deleted when the parent gets deleted)
         */
        explicit AbstractResourcesBackend(QObject* parent = 0);
        
        /**
         * @returns true when the backend is in a valid state, which means it is able to work
         * You must return true here if you want the backend to be loaded.
         */
        virtual bool isValid() const = 0;
        
        /**
         * @returns all resources of the backend
         */
        virtual QVector<AbstractResource*> allResources() const = 0;
        
        /**
         * In this method the backend should search in each resources name if it complies
         * to the searchText and return those AbstractResources.
         * @returns the list of resources whose name contains searchText
         */
        virtual QList<AbstractResource*> searchPackageName(const QString &searchText) = 0;//FIXME: Probably provide a standard implementation?!
        
        /**
         * @returns the reviews backend of this AbstractResourcesBackend (which handles all ratings and reviews of resources)
         */
        virtual AbstractReviewsBackend* reviewsBackend() const = 0;//FIXME: Have a standard impl which returns 0?
        
        /**
         * @returns the class which is used by muon to update the users system, if you are unsure what to do
         * just return the StandardBackendUpdater
         */
        virtual AbstractBackendUpdater* backendUpdater() const = 0;//FIXME: Standard impl returning the standard updater?
        
        /**
         * @returns the number of resources for which an update is available, it should only count technical packages
         */
        virtual int updatesCount() const = 0;//FIXME: Probably provide a standard implementation?!
        
        /**
         * Gets a resource identified by the name
         * @param name the name to search for
         * @returns the resource with the provided name
         */
        virtual AbstractResource* resourceByPackageName(const QString& name) const = 0;//FIXME: Even this could get a standard impl
        
        /**
         * @returns all resources for which an update is available
         */
        virtual QList<AbstractResource*> upgradeablePackages() const = 0;//FIXME: Do a standard impl as well
        
        /**
         * This method gets called while initializing the GUI, in case the backend needs to
         * integrate in a special way with the MuonMainWindow.
         * @param w the MuonMainWindow the backend should integrate to
         */
        virtual void integrateMainWindow(MuonMainWindow* w);

        /**
         * Tells whether the backend is fetching resources
         */
        virtual bool isFetching() const = 0;

    public slots:
        /**
         * This gets called when the backend should install an application.
         * The AbstractResourcesBackend should create a Transaction object, which
         * will provide Muon with information like the status and progress of a transaction,
         * and add it to the TransactionModel with the following line:
         * \code
         * TransactionModel::global()->addTransaction(transaction);
         * \endcode
         * where transaction is the newly created Transaction.
         * @param app the application to be installed
         * @param addons the addons which should be installed with the application
         */
        virtual void installApplication(AbstractResource *app, AddonList addons) = 0;
        
        /**
         * Overloaded function, which simply does the same, except not installing any addons.
         */
        virtual void installApplication(AbstractResource *app);
        
        /**
         * This gets called when the backend should remove an application.
         * Like in the installApplication() method, the AbstractResourcesBackend should
         * create a Transaction object and add it to the TransactionModel.
         * @see installApplication
         * @param app the application to be removed
         */
        virtual void removeApplication(AbstractResource *app) = 0;
        
        /**
         * This gets called when a transaction should get canceled and thus the backend
         * should cancel the transaction and remove it from the TransactionModel:
         * \code
         * TransactionModel::global()->removeTransaction(t);
         * \endcode
         * @param app the application whose transaction is going to be canceled
         */
        virtual void cancelTransaction(AbstractResource *app) = 0;

    signals:
        /**
         * Notify of a change in the backend
         */
        void fetchingChanged();

        /**
         * This should be emitted when the number of upgradeable packages changed.
         */
        void updatesCountChanged();
        /**
         * This should be emitted when all data of the backends resources changed. Internally it will emit
         * a signal in the model to show the view that all data of a certain backend changed.
         */
        void allDataChanged();
        /**
         * This should be emitted whenever there are new search results available, other than the ones returned previously,
         * or the data set in which the backend searched changed.
         */
        void searchInvalidated();
};

Q_DECLARE_INTERFACE( AbstractResourcesBackend, "org.kde.muon.AbstractResourcesBackend" )

#endif // ABSTRACTRESOURCESBACKEND_H
