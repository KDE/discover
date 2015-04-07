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

#ifndef PACKAGEKITBACKEND_H
#define PACKAGEKITBACKEND_H

#include "PackageKitResource.h"
#include <resources/AbstractResourcesBackend.h>
#include <QVariantList>
#include <QStringList>
#include <qpointer.h>
#include <QSet>
#include <PackageKit/Transaction>
#include <AppstreamQt/database.h>
#include <functional>

class PackageKitUpdater;

class MUONCOMMON_EXPORT PackageKitBackend : public AbstractResourcesBackend
{
    Q_OBJECT
    public:
        explicit PackageKitBackend(QObject* parent = 0);
        ~PackageKitBackend();
        
        virtual AbstractBackendUpdater* backendUpdater() const;
        virtual AbstractReviewsBackend* reviewsBackend() const;
        
        virtual QVector< AbstractResource* > allResources() const;
        virtual AbstractResource* resourceByPackageName(const QString& name) const;
        virtual QList<AbstractResource*> searchPackageName(const QString& searchText);
        virtual int updatesCount() const;
        
        virtual void installApplication(AbstractResource* app);
        virtual void installApplication(AbstractResource* app, AddonList addons);
        virtual void removeApplication(AbstractResource* app);
        virtual void cancelTransaction(AbstractResource* app);
        virtual bool isValid() const { return true; }
        virtual QList<AbstractResource*> upgradeablePackages() const;
        virtual bool isFetching() const;
        virtual QList<QAction*> messageActions() const;

        bool isPackageNameUpgradeable(const PackageKitResource* res) const;
        QString upgradeablePackageId(const PackageKitResource* res) const;
        QVector<AbstractResource*> resourcesByPackageName(const QString& name, bool updating) const;

    public slots:
        void removeTransaction(Transaction* t);
        void reloadPackageList();
        void refreshDatabase();

    private slots:
        void getPackagesFinished(PackageKit::Transaction::Exit exit);
        void addPackage(PackageKit::Transaction::Info info, const QString &packageId, const QString &summary);
        void packageDetails(const PackageKit::Details& details);
        void transactionError(PackageKit::Transaction::Error, const QString& message);
        void addPackageToUpdate(PackageKit::Transaction::Info, const QString& pkgid, const QString& summary);
        void getUpdatesFinished(PackageKit::Transaction::Exit,uint);
        void getUpdatesDetailsFinished(PackageKit::Transaction::Exit,uint);

    private:
        void checkDaemonRunning();
        void fetchUpdates();
        void acquireFetching(bool f);

        QHash<QString, AbstractResource*> m_packages;
        QHash<QString, AbstractResource*> m_updatingPackages;
        Appstream::Database m_appdata;
        QList<Transaction*> m_transactions;
        PackageKitUpdater* m_updater;
        QPointer<PackageKit::Transaction> m_refresher;
        int m_isFetching;
        QSet<QString> m_updatesPackageId;
        QList<QAction*> m_messageActions;
        QHash<QString, QStringList> m_translationPackageToApp;
        QHash<QString, QStringList> m_updatingTranslationPackageToApp;
};

#endif // PACKAGEKITBACKEND_H
