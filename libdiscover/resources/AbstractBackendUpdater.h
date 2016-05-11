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

#ifndef ABSTRACTBACKENDUPDATER_H
#define ABSTRACTBACKENDUPDATER_H

#include <QObject>
#include "discovercommon_export.h"

class QAction;
class QDateTime;
class QIcon;
class AbstractResource;

/**
 * \class AbstractBackendUpdater  AbstractBackendUpdater.h "AbstractBackendUpdater.h"
 *
 * \brief This is the base class for all abstract classes, which handle system upgrades.
 * 
 * While implementing this is not mandatory for all backends (you can also use the 
 * StandardBackendUpdater, which just uses the functions in the ResourcesBackend to
 * update the packages), it is recommended for many.
 * 
 * Before starting the update, the AbstractBackendUpdater will have to keep a list of
 * packages, which are about to be upgraded. First, all packages have to be inserted
 * into this list in the \prepare method and they can then be changed by the user through 
 * the \addResources and \removeResources functions.
 * 
 * When \start is called, the AbstractBackendUpdater should start the update and report its
 * progress through the rest of methods outlined in this API documentation.
 * 
 * @see addResources
 * @see removeResources
 * @see start
 * @see prepare
 */
class DISCOVERCOMMON_EXPORT AbstractBackendUpdater : public QObject
{
    Q_OBJECT
    Q_PROPERTY(qreal progress READ progress NOTIFY progressChanged)
    Q_PROPERTY(bool isCancelable READ isCancelable NOTIFY cancelableChanged)
    Q_PROPERTY(bool isProgressing READ isProgressing NOTIFY progressingChanged)
    Q_PROPERTY(QString statusMessage READ statusMessage NOTIFY statusMessageChanged)
    Q_PROPERTY(QString statusDetail READ statusDetail NOTIFY statusDetailChanged)
    Q_PROPERTY(quint64 downloadSpeed READ downloadSpeed NOTIFY downloadSpeedChanged)
    public:
        /**
         * Constructs an AbstractBackendUpdater
         */
        explicit AbstractBackendUpdater(QObject* parent = nullptr);
        
        /**
         * This method is called, when Muon switches to the updates view.
         * Here the backend should mark all upgradeable packages as to be upgraded.
         */
        virtual void prepare() = 0;
        
        /**
         * @returns true if the backend contains packages which can be updated
         */
        virtual bool hasUpdates() const = 0;
        /**
         * @returns the progress of the update in percent
         */
        virtual qreal progress() const = 0;
        
        /** 
         * @returns the proposed ETA in milliseconds 
         */
        virtual long unsigned int remainingTime() const = 0;
        
        /**
         * This method is used to remove resources from the list of packages
         * marked to be upgraded. It will potentially be called before \start.
         */
        virtual void removeResources(const QList<AbstractResource*>& apps) = 0;
        /**
         * This method is used to add resource to the list of packages marked to be upgraded.
         * It will potentially be called before \start.
         */
        virtual void addResources(const QList<AbstractResource*>& apps) = 0;

        /**
         * @returns the list of updateable resources in the system
         */
        virtual QList<AbstractResource*> toUpdate() const = 0;

        /**
         * @returns the QDateTime when the last update happened
         */
        virtual QDateTime lastUpdate() const = 0;

        /**
         * @returns whether the updater can currently be canceled or not
         * @see cancelableChanged
         */
        virtual bool isCancelable() const = 0;
        /**
         * @returns whether the updater is currently running or not
         * this property decides, if there will be progress reporting in the GUI.
         * This has to stay true during the whole transaction!
         * @see progressingChanged
         */
        virtual bool isProgressing() const = 0;
        
        /**
         * @returns the string about the current status of the update
         */
        virtual QString statusMessage() const = 0;
        /**
         * @returns a more detailled description of what is currently happening with the update
         */
        virtual QString statusDetail() const = 0;
        /**
         * @returns the overall download speed during the downloading phase of the update
         */
        virtual quint64 downloadSpeed() const = 0;

        /**
         * @returns whether @p res is marked for update
         */
        virtual bool isMarked(AbstractResource* res) const = 0;

    public Q_SLOTS:
        /**
         * If \isCancelable is true during the transaction, this method has
         * to be implemented and will potentially be called when the user
         * wants to cancel the update.
         */
        virtual void cancel();
        /**
         * This method starts the update. All packages which are in \toUpdate
         * are going to be updated.
         * 
         * From this moment on the AbstractBackendUpdater should continuously update
         * the other methods to show its progress.
         * 
         * @see progress
         * @see progressChanged
         * @see isProgressing
         * @see progressingChanged
         */
        virtual void start() = 0;

    Q_SIGNALS:
        /**
         * The AbstractBackendUpdater should emit this signal when the progress changed.
         * @see progress
         */
        void progressChanged(qreal progress);
        /**
         * The AbstractBackendUpdater should emit this signal when the remaining time changed.
         * @see remainingTime
         */
        void remainingTimeChanged();//FIXME: API inconsistency here!!
        /**
         * The AbstractBackendUpdater should emit this signal when the cancelable property changed.
         * @see isCancelable
         */
        void cancelableChanged(bool cancelable);
        /**
         * The AbstractBackendUpdater should emit this signal when the progressing property changed.
         * @see isProgressing
         */
        void progressingChanged(bool progressing);
        /**
         * The AbstractBackendUpdater should emit this signal when the status detail changed.
         * @see statusDetail
         */
        void statusDetailChanged(const QString& msg);
        /**
         * The AbstractBackendUpdater should emit this signal when the status message changed.
         * @see statusMessage
         */
        void statusMessageChanged(const QString& msg);
        /**
         * The AbstractBackendUpdater should emit this signal when the download speed changed.
         * @see downloadSpeed
         */
        void downloadSpeedChanged(quint64);

        /**
         * Provides the @p progress of a specific @p resource in a percentage.
         */
        void resourceProgressed(AbstractResource* resource, qreal progress);
};

#endif // ABSTRACTBACKENDUPDATER_H
