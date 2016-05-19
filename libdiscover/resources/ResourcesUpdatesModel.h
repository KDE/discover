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

#ifndef RESOURCESUPDATESMODEL_H
#define RESOURCESUPDATESMODEL_H

#include <QStandardItemModel>
#include <QDateTime>
#include "discovercommon_export.h"

class AbstractResourcesBackend;
class AbstractResource;
class QAction;
class AbstractBackendUpdater;
class ResourcesModel;
class QDBusInterface;
class Transaction;

class DISCOVERCOMMON_EXPORT ResourcesUpdatesModel : public QStandardItemModel
{
    Q_OBJECT
    Q_PROPERTY(qreal progress READ progress NOTIFY progressChanged)
    Q_PROPERTY(QString remainingTime READ remainingTime NOTIFY etaChanged)
    Q_PROPERTY(quint64 downloadSpeed READ downloadSpeed NOTIFY downloadSpeedChanged)
    Q_PROPERTY(bool isCancelable READ isCancelable NOTIFY cancelableChanged)
    Q_PROPERTY(bool isProgressing READ isProgressing NOTIFY progressingChanged)
    Q_PROPERTY(QDateTime lastUpdate READ lastUpdate NOTIFY progressingChanged)
    Q_PROPERTY(qint64 secsToLastUpdate READ secsToLastUpdate NOTIFY progressingChanged)
    public:
        explicit ResourcesUpdatesModel(QObject* parent = nullptr);
        
        qreal progress() const;
        QString remainingTime() const;
        quint64 downloadSpeed() const;
        Q_SCRIPTABLE void prepare();
        void fetchChangelog();

        ///checks if any of them is cancelable
        bool isCancelable() const;
        bool isProgressing() const;
        QList<AbstractResource*> toUpdate() const;
        QDateTime lastUpdate() const;
        void addResources(const QList<AbstractResource*>& resources);
        void removeResources(const QList<AbstractResource*>& resources);

        qint64 secsToLastUpdate() const;

    Q_SIGNALS:
        void downloadSpeedChanged();
        void progressChanged();
        void etaChanged();
        void cancelableChanged();
        void progressingChanged(bool progressing);
        void statusMessageChanged(const QString& message);
        void statusDetailChanged(const QString& msg);
        void finished();
        void resourceProgressed(AbstractResource* resource, qreal progress);

    public Q_SLOTS:
        void cancel();
        void updateAll();

    private Q_SLOTS:
        void updaterDestroyed(QObject* obj);
        void message(const QString& msg);
        void slotProgressingChanged(bool progressing);

    private:
        void init();

        QVector<AbstractBackendUpdater*> m_updaters;
        bool m_lastIsProgressing;
        Transaction* m_transaction;
};

#endif // RESOURCESUPDATESMODEL_H
