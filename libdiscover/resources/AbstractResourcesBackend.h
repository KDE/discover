/***************************************************************************
 *   Copyright ?? 2012 Aleix Pol Gonzalez <aleixpol@blue-systems.com>       *
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

#include "AbstractResource.h"
#include "Transaction/AddonList.h"

#include "discovercommon_export.h"

class Transaction;
class Category;
class AbstractReviewsBackend;
class AbstractBackendUpdater;

class DISCOVERCOMMON_EXPORT ResultsStream : public QObject
{
    Q_OBJECT
    public:
        ResultsStream(const QString &objectName);

        /// assumes all the information is in @p resources
        ResultsStream(const QString &objectName, const QVector<AbstractResource*>& resources);
        ~ResultsStream() override;

        void finish();

    Q_SIGNALS:
        void resourcesFound(const QVector<AbstractResource*>& resources);
        void fetchMore();
};

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
class DISCOVERCOMMON_EXPORT AbstractResourcesBackend : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString name READ name CONSTANT)
    Q_PROPERTY(QString displayName READ displayName CONSTANT)
    Q_PROPERTY(AbstractReviewsBackend* reviewsBackend READ reviewsBackend CONSTANT)
    Q_PROPERTY(int updatesCount READ updatesCount NOTIFY updatesCountChanged)
    Q_PROPERTY(bool hasSecurityUpdates READ hasSecurityUpdates NOTIFY updatesCountChanged)
    Q_PROPERTY(bool isFetching READ isFetching NOTIFY fetchingChanged)
    Q_PROPERTY(bool hasApplications READ hasApplications CONSTANT)
    public:
        /**
         * Constructs an AbstractResourcesBackend
         * @param parent the parent of the class (the object will be deleted when the parent gets deleted)
         */
        explicit AbstractResourcesBackend(QObject* parent = nullptr);
        
        /**
         * @returns true when the backend is in a valid state, which means it is able to work
         * You must return true here if you want the backend to be loaded.
         */
        virtual bool isValid() const = 0;
        
        struct Filters {
            Category* category = nullptr;
            AbstractResource::State state = AbstractResource::Broken;
            QString mimetype;
            QString search;
            QString extends;
            QUrl resourceUrl;
            QString origin;
            bool allBackends = false;
            bool filterMinimumState = true;

            bool isEmpty() const { return !category && state == AbstractResource::Broken && mimetype.isEmpty() && search.isEmpty() && extends.isEmpty() && resourceUrl.isEmpty() && origin.isEmpty(); }

            bool shouldFilter(AbstractResource* res) const;
            void filterJustInCase(QVector<AbstractResource*>& input) const;
        };

        /**
         * @returns a stream that will provide elements that match the search
         */

        virtual ResultsStream* search(const Filters &search) = 0;//FIXME: Probably provide a standard implementation?!

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
         * @returns whether either of the updates contains a security fix
         */
        virtual bool hasSecurityUpdates() const { return false; }

        /**
         * Tells whether the backend is fetching resources
         */
        virtual bool isFetching() const = 0;

        /**
         * @returns the appstream ids that this backend extends
         */
        virtual QStringList extends() const;

        /** @returns the plugin's name */
        QString name() const;

        /** @internal only to be used by the factory */
        void setName(const QString& name);

        virtual QString displayName() const = 0;

        /**
         * emits a change for all rating properties
         */
        void emitRatingsReady();

        virtual AbstractResource* resourceForFile(const QUrl &/*url*/) { return nullptr; }

        /**
         * @returns the root category tree
         */
        virtual QVector<Category*> category() const { return {}; }

        virtual bool hasApplications() const { return false; }

    public Q_SLOTS:
        /**
         * This gets called when the backend should install an application.
         * The AbstractResourcesBackend should create a Transaction object, is returned and
         * will be included in the TransactionModel
         * @param app the application to be installed
         * @param addons the addons which should be installed with the application
         * @returns the Transaction that keeps track of the installation process
         */
        virtual Transaction* installApplication(AbstractResource *app, const AddonList& addons) = 0;
        
        /**
         * Overloaded function, which simply does the same, except not installing any addons.
         */
        virtual Transaction* installApplication(AbstractResource *app);
        
        /**
         * This gets called when the backend should remove an application.
         * Like in the installApplication() method, we'll return the Transaction
         * responsible for the removal.
         *
         * @see installApplication
         * @param app the application to be removed
         * @returns the Transaction that keeps track of the removal process
         */
        virtual Transaction* removeApplication(AbstractResource *app) = 0;

        /**
         * Notifies the backend that the user wants the information to be up to date
         */
        virtual void checkForUpdates() = 0;

    Q_SIGNALS:
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
        void allDataChanged(const QVector<QByteArray> &propertyNames);

        /**
         * Allows to notify some @p properties in @p resource have changed
         */
        void resourcesChanged(AbstractResource* resource, const QVector<QByteArray> &properties);
        void resourceRemoved(AbstractResource* resource);

        void passiveMessage(const QString &message);

    private:
        QString m_name;
};

DISCOVERCOMMON_EXPORT QDebug operator<<(QDebug dbg, const AbstractResourcesBackend::Filters& filters);

/**
 * @internal Workaround because QPluginLoader enforces 1 instance per plugin
 */
class DISCOVERCOMMON_EXPORT AbstractResourcesBackendFactory : public QObject
{
    Q_OBJECT
public:
    virtual QVector<AbstractResourcesBackend*> newInstance(QObject* parent, const QString &name) const = 0;
};

#define DISCOVER_BACKEND_PLUGIN(ClassName)\
    class ClassName##Factory : public AbstractResourcesBackendFactory {\
        Q_OBJECT\
        Q_PLUGIN_METADATA(IID "org.kde.muon.AbstractResourcesBackendFactory")\
        Q_INTERFACES(AbstractResourcesBackendFactory)\
        public:\
            QVector<AbstractResourcesBackend*> newInstance(QObject* parent, const QString &name) const override {\
                auto c = new ClassName(parent);\
                c->setName(name);\
                return {c};\
            }\
    };

Q_DECLARE_INTERFACE( AbstractResourcesBackendFactory, "org.kde.muon.AbstractResourcesBackendFactory" )

#endif // ABSTRACTRESOURCESBACKEND_H
