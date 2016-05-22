/*
 *   Copyright (C) 2012 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library/Lesser General Public License
 *   version 2, or (at your option) any later version, as published by the
 *   Free Software Foundation
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details
 *
 *   You should have received a copy of the GNU Library/Lesser General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef STANDARDBACKENDUPDATER_H
#define STANDARDBACKENDUPDATER_H

#include "discovercommon_export.h"
#include <resources/AbstractBackendUpdater.h>
#include "AbstractResourcesBackend.h"
#include <QSet>
#include <QDateTime>

class AbstractResourcesBackend;

class DISCOVERCOMMON_EXPORT StandardBackendUpdater : public AbstractBackendUpdater
{
    Q_OBJECT
    public:
        explicit StandardBackendUpdater(AbstractResourcesBackend* parent = nullptr);

        bool hasUpdates() const override;
        qreal progress() const override;
        void start() override;
        long unsigned int remainingTime() const override;
        
        QList<AbstractResource*> toUpdate() const override;
        void addResources(const QList<AbstractResource*>& apps) override;
        void removeResources(const QList<AbstractResource*>& apps) override;
        void prepare() override;
        QDateTime lastUpdate() const override;
        bool isCancelable() const override;
        bool isProgressing() const override;
        QString statusDetail() const override;
        QString statusMessage() const override;
        quint64 downloadSpeed() const override;
        virtual QList<QAction*> messageActions() const;
        bool isMarked(AbstractResource* res) const override;
        void setStatusDetail(const QString& message);
        void setProgress(qreal p);
        int updatesCount() const;

        void setMessageActions(const QList<QAction*>& actions);

    public Q_SLOTS:
        void transactionRemoved(Transaction* t);
        void cleanup();

    private:
        void refreshUpdateable();
        void transactionAdded(Transaction* newTransaction);
        void transactionProgressChanged(int percentage);

        QSet<AbstractResource*> m_toUpgrade;
        QSet<AbstractResource*> m_upgradeable;
        AbstractResourcesBackend * const m_backend;
        QSet<AbstractResource*> m_pendingResources;
        bool m_settingUp;
        QString m_statusDetail;
        qreal m_progress;
        QDateTime m_lastUpdate;
        QList<QAction*> m_actions;
};

#endif // STANDARDBACKENDUPDATER_H

