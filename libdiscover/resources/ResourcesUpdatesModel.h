/*
 *   SPDX-FileCopyrightText: 2012 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#ifndef RESOURCESUPDATESMODEL_H
#define RESOURCESUPDATESMODEL_H

#include <QStandardItemModel>
#include <QDateTime>
#include <QPointer>
#include "discovercommon_export.h"
#include "resources/AbstractBackendUpdater.h"

class AbstractResource;
class UpdateTransaction;
class Transaction;

class DISCOVERCOMMON_EXPORT ResourcesUpdatesModel : public QStandardItemModel
{
    Q_OBJECT
    Q_PROPERTY(bool isProgressing READ isProgressing NOTIFY progressingChanged)
    Q_PROPERTY(QDateTime lastUpdate READ lastUpdate NOTIFY progressingChanged)
    Q_PROPERTY(qint64 secsToLastUpdate READ secsToLastUpdate NOTIFY progressingChanged)
    Q_PROPERTY(Transaction* transaction READ transaction NOTIFY progressingChanged)
    Q_PROPERTY(bool needsReboot READ needsReboot NOTIFY needsRebootChanged)
    public:
        explicit ResourcesUpdatesModel(QObject* parent = nullptr);

        Q_SCRIPTABLE void prepare();

        bool isProgressing() const;
        QList<AbstractResource*> toUpdate() const;
        QDateTime lastUpdate() const;
        double updateSize() const;
        void addResources(const QList<AbstractResource*>& resources);
        void removeResources(const QList<AbstractResource*>& resources);

        qint64 secsToLastUpdate() const;
        QVector<AbstractBackendUpdater*> updaters() const { return m_updaters; }
        Transaction* transaction() const;
        bool needsReboot() const;

    Q_SIGNALS:
        void downloadSpeedChanged();
        void progressingChanged();
        void finished();
        void resourceProgressed(AbstractResource* resource, qreal progress, AbstractBackendUpdater::State state);
        void passiveMessage(const QString &message);
        void needsRebootChanged();
        void fetchingUpdatesProgressChanged(int percent);

    public Q_SLOTS:
        void updateAll();

    private Q_SLOTS:
        void updaterDestroyed(QObject* obj);
        void message(const QString& msg);

    private:
        void init();
        void setTransaction(UpdateTransaction* transaction);

        QVector<AbstractBackendUpdater*> m_updaters;
        bool m_lastIsProgressing;
        QPointer<UpdateTransaction> m_transaction;
};

#endif // RESOURCESUPDATESMODEL_H
