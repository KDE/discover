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
#include "libmuonprivate_export.h"

class QAction;
class QDateTime;
class QIcon;
class AbstractResource;
class MUONPRIVATE_EXPORT AbstractBackendUpdater : public QObject
{
    Q_OBJECT
    Q_PROPERTY(qreal progress READ progress NOTIFY progressChanged)
    Q_PROPERTY(bool isCancelable READ isCancelable NOTIFY cancelableChanged)
    Q_PROPERTY(bool isProgressing READ isProgressing NOTIFY progressingChanged)
    Q_PROPERTY(QString statusMessage READ statusMessage NOTIFY statusMessageChanged)
    Q_PROPERTY(QString statusDetail READ statusDetail NOTIFY statusDetailChanged)
    Q_PROPERTY(quint64 downloadSpeed READ downloadSpeed NOTIFY downloadSpeedChanged)
    public:
        explicit AbstractBackendUpdater(QObject* parent = 0);
        
        virtual void prepare() = 0;
        
        virtual bool hasUpdates() const = 0;
        virtual qreal progress() const = 0;
        
        /** proposed ETA in milliseconds */
        virtual long unsigned int remainingTime() const = 0;
        
        virtual void removeResources(const QList<AbstractResource*>& apps) = 0;
        virtual void addResources(const QList<AbstractResource*>& apps) = 0;
        virtual QList<AbstractResource*> toUpdate() const = 0;
        virtual QDateTime lastUpdate() const = 0;
        virtual bool isAllMarked() const = 0;
        virtual bool isCancelable() const = 0;
        virtual bool isProgressing() const = 0;
        
        virtual QString statusMessage() const = 0;
        virtual QString statusDetail() const = 0;
        virtual quint64 downloadSpeed() const = 0;

        /** in muon-updater, actions with HighPriority will be shown in a KMessageWidget,
         *  normal priority will go right on top of the more menu, low priority will go
         *  to the advanced menu
         */
        virtual QList<QAction*> messageActions() const = 0;

    public slots:
        ///must be implemented if ever isCancelable is true
        virtual void cancel();
        virtual void start() = 0;

    signals:
        void progressChanged(qreal progress);
        void remainingTimeChanged();//FIXME: API inconsistency here!!
        void cancelableChanged(bool cancelable);
        void progressingChanged(bool progressing);
        void statusDetailChanged(const QString& msg);
        void statusMessageChanged(const QString& msg);
        void downloadSpeedChanged(quint64);
};

#endif // ABSTRACTBACKENDUPDATER_H

