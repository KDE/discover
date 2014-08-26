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
#include <packagekitqt5/Transaction>
#include <AppstreamQt/database.h>

class PackageKitUpdater;

class MUONPRIVATE_EXPORT PackageKitBackend : public AbstractResourcesBackend
{
    Q_OBJECT
    Q_INTERFACES(AbstractResourcesBackend)
    public:
        explicit PackageKitBackend(QObject* parent, const QVariantList& args);
        ~PackageKitBackend();
        
        static QString errorMessage(PackageKit::Transaction::Error error);
        
        virtual AbstractBackendUpdater* backendUpdater() const;
        virtual AbstractReviewsBackend* reviewsBackend() const;
        
        virtual QVector< AbstractResource* > allResources() const;
        virtual AbstractResource* resourceByPackageName(const QString& name) const;
        virtual QList<AbstractResource*> searchPackageName(const QString& searchText);
        virtual int updatesCount() const;
        int allUpdatesCount() const;
        
        virtual void installApplication(AbstractResource* app);
        virtual void installApplication(AbstractResource* app, AddonList addons);
        virtual void removeApplication(AbstractResource* app);
        virtual void cancelTransaction(AbstractResource* app);
        virtual bool isValid() const { return true; }
        virtual QList<AbstractResource*> upgradeablePackages() const;
        virtual bool isFetching() const;

    public slots:
        void removeTransaction(Transaction* t);
        void populateInstalledCache();

    private slots:
        void getPackagesFinished();
        void updateDatabase();
        void addPackage(PackageKit::Transaction::Info info, const QString &packageId, const QString &summary);
        void packageDetails(const PackageKit::Details& details);
        void getDetailsFinished(PackageKit::Transaction::Exit, uint);

    private:
        void setFetching(bool f);

        QHash<QString, AbstractResource*> m_packages;
        QHash<QString, AbstractResource*> m_updatingPackages;
        Appstream::Database m_appdata;
        QList<Transaction*> m_transactions;
        PackageKitUpdater* m_updater;
        QList<PackageKitResource*> m_upgradeablePackages;
        QPointer<PackageKit::Transaction> m_refresher;
        bool m_isFetching;
};

#endif // PACKAGEKITBACKEND_H
