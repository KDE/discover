/***************************************************************************
 *   Copyright © 2013 Lukas Appelhans <l.appelhans@gmx.de>                 *
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
#ifndef PACKAGEKITUPDATER_H
#define PACKAGEKITUPDATER_H

#include <resources/AbstractBackendUpdater.h>
#include "PackageKitBackend.h"
#include <PackageKit/Transaction>

class PackageKitUpdater : public AbstractBackendUpdater
{
    Q_OBJECT
    public:
        PackageKitUpdater(PackageKitBackend * parent = nullptr);
        ~PackageKitUpdater();
        
        virtual void prepare() override;
        
        virtual bool hasUpdates() const override;
        virtual qreal progress() const override;
        
        /** proposed ETA in milliseconds */
        virtual long unsigned int remainingTime() const override;
        
        virtual void removeResources(const QList<AbstractResource*>& apps) override;
        virtual void addResources(const QList<AbstractResource*>& apps) override;
        virtual QList<AbstractResource*> toUpdate() const override;
        virtual bool isMarked(AbstractResource* res) const override;
        virtual QDateTime lastUpdate() const override;
        virtual bool isAllMarked() const override;
        virtual bool isCancelable() const override;
        virtual bool isProgressing() const override;
        
        virtual QString statusMessage() const override;
        virtual QString statusDetail() const override;
        virtual quint64 downloadSpeed() const override;

        /** in muon-updater, actions with HighPriority will be shown in a KMessageWidget,
         *  normal priority will go right on top of the more menu, low priority will go
         *  to the advanced menu
         */
        virtual QList<QAction*> messageActions() const;

    public slots:
        ///must be implemented if ever isCancelable is true
        virtual void cancel() override;
        virtual void start() override;
    
    private slots:
        void errorFound(PackageKit::Transaction::Error err, const QString& error);
        void mediaChange(PackageKit::Transaction::MediaType media, const QString& type, const QString& text);
        void requireRestart(PackageKit::Transaction::Restart restart, const QString& p);
        void eulaRequired(const QString &eulaID, const QString &packageID, const QString &vendor, const QString &licenseAgreement);
        void finished(PackageKit::Transaction::Exit exit, uint);
        void statusChanged();
        void speedChanged();
        void cancellableChanged();
        void remainingTimeChanged();
        void percentageChanged();
        
    private:
        void fetchLastUpdateTime();
        void lastUpdateTimeReceived(QDBusPendingCallWatcher* w);
        void setProgressing(bool progressing);
        void setTransaction(PackageKit::Transaction* transaction);
        QSet<QString> involvedPackages(const QSet<AbstractResource*>& packages) const;
        QSet<AbstractResource*> packagesForPackageId(const QSet<QString>& packages) const;

        QPointer<PackageKit::Transaction> m_transaction;
        PackageKitBackend * m_backend;
        QSet<AbstractResource*> m_toUpgrade;
        QSet<AbstractResource*> m_allUpgradeable;
        bool m_isCancelable;
        bool m_isProgressing;
        PackageKit::Transaction::Status m_status;
        QString m_statusMessage;
        QString m_statusDetail;
        quint64 m_speed;
        long unsigned int m_remainingTime;
        uint m_percentage;
        QAction* m_updateAction;
        QDateTime m_lastUpdate;
};


#endif
