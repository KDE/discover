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

        virtual bool hasUpdates() const override;
        virtual qreal progress() const override;
        virtual void start() override;
        virtual long unsigned int remainingTime() const override;
        
        virtual QList<AbstractResource*> toUpdate() const override;
        virtual void addResources(const QList<AbstractResource*>& apps) override;
        virtual void removeResources(const QList<AbstractResource*>& apps) override;
        virtual void prepare() override;
        virtual bool isAllMarked() const override;
        virtual QDateTime lastUpdate() const override;
        virtual bool isCancelable() const override;
        virtual bool isProgressing() const override;
        virtual QString statusDetail() const override;
        virtual QString statusMessage() const override;
        virtual quint64 downloadSpeed() const override;
        virtual QList<QAction*> messageActions() const;
        virtual bool isMarked(AbstractResource* res) const override;
        void setStatusDetail(const QString& message);
        void setProgress(qreal p);

        void setMessageActions(const QList<QAction*>& actions);

    public slots:
        void transactionRemoved(Transaction* t);
        void cleanup();

    private:
        QSet<AbstractResource*> m_toUpgrade;
        AbstractResourcesBackend* m_backend;
        int m_preparedSize;
        QSet<AbstractResource*> m_pendingResources;
        bool m_settingUp;
        QString m_statusDetail;
        qreal m_progress;
        QDateTime m_lastUpdate;
        QList<QAction*> m_actions;
};

#endif // STANDARDBACKENDUPDATER_H

