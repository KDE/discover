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

#include "libmuonprivate_export.h"
#include <resources/AbstractBackendUpdater.h>
#include "AbstractResourcesBackend.h"
#include <QSet>
#include <QDateTime>

class AbstractResourcesBackend;

class MUONPRIVATE_EXPORT StandardBackendUpdater : public AbstractBackendUpdater
{
    Q_OBJECT
    public:
        explicit StandardBackendUpdater(AbstractResourcesBackend* parent = 0);

        virtual bool hasUpdates() const;
        virtual qreal progress() const;
        virtual void start();
        virtual long unsigned int remainingTime() const;
        
        virtual QList<AbstractResource*> toUpdate() const;
        virtual void addResources(const QList<AbstractResource*>& apps);
        virtual void removeResources(const QList<AbstractResource*>& apps);
        virtual void prepare();
        virtual bool isAllMarked() const;
        virtual QDateTime lastUpdate() const;
        virtual bool isCancelable() const;
        virtual bool isProgressing() const;
        virtual QString statusDetail() const;
        virtual QString statusMessage() const;
        virtual quint64 downloadSpeed() const;
        virtual QList<QAction*> messageActions() const;
        virtual bool isMarked(AbstractResource* res) const;
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

