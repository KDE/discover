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
#include <QPointer>
#include <QTimer>
#include <QSet>
#include <QSharedPointer>
#include <PackageKit/Transaction>
#include <AppStreamQt/pool.h>

class AppPackageKitResource;
class PackageKitUpdater;
class PKTransaction;
class OdrsReviewsBackend;
class DISCOVERCOMMON_EXPORT PackageKitBackend : public AbstractResourcesBackend
{
    Q_OBJECT
    public:
        explicit PackageKitBackend(QObject* parent = nullptr);
        ~PackageKitBackend() override;

        AbstractBackendUpdater* backendUpdater() const override;
        AbstractReviewsBackend* reviewsBackend() const override;
        QSet<AbstractResource*> resourcesByPackageName(const QString& name) const;

        ResultsStream* search(const AbstractResourcesBackend::Filters & search) override;
        ResultsStream* findResourceByPackageName(const QUrl& search) override;
        int updatesCount() const override;
        bool hasSecurityUpdates() const override;

        Transaction* installApplication(AbstractResource* app) override;
        Transaction* installApplication(AbstractResource* app, const AddonList& addons) override;
        Transaction* removeApplication(AbstractResource* app) override;
        bool isValid() const override { return true; }
        QSet<AbstractResource*> upgradeablePackages() const;
        bool isFetching() const override;

        bool isPackageNameUpgradeable(const PackageKitResource* res) const;
        QString upgradeablePackageId(const PackageKitResource* res) const;
        QVector<AppPackageKitResource*> extendedBy(const QString& id) const;

        void clearPackages(const QStringList &packageNames);
        void resolvePackages(const QStringList &packageNames);
        void fetchDetails(const QString& pkgid);

        AbstractResource * resourceForFile(const QUrl & ) override;
        void checkForUpdates() override;
        QString displayName() const override;

        bool hasApplications() const override { return true; }
        static QString locateService(const QString &filename);

    public Q_SLOTS:
        void reloadPackageList();
        void refreshDatabase();

    private Q_SLOTS:
        void getPackagesFinished();
        void addPackage(PackageKit::Transaction::Info info, const QString &packageId, const QString &summary, bool arch);
        void addPackageArch(PackageKit::Transaction::Info info, const QString &packageId, const QString &summary);
        void addPackageNotArch(PackageKit::Transaction::Info info, const QString &packageId, const QString &summary);
        void packageDetails(const PackageKit::Details& details);
        void transactionError(PackageKit::Transaction::Error, const QString& message);
        void addPackageToUpdate(PackageKit::Transaction::Info, const QString& pkgid, const QString& summary);
        void getUpdatesFinished(PackageKit::Transaction::Exit,uint);
        void getUpdatesDetailsFinished(PackageKit::Transaction::Exit,uint);

    private:
        template <typename T, typename Q>
        T resourcesByPackageNames(const Q& names) const;
        void fetchUpdates();

        void checkDaemonRunning();
        void acquireFetching(bool f);
        void includePackagesToAdd();
        void performDetailsFetch();
        AppPackageKitResource* addComponent(const AppStream::Component& component, const QStringList& pkgNames);

        AppStream::Pool m_appdata;
        PackageKitUpdater* m_updater;
        QPointer<PackageKit::Transaction> m_refresher;
        int m_isFetching;
        QSet<QString> m_updatesPackageId;
        bool m_hasSecurityUpdates = false;
        QSet<PackageKitResource*> m_packagesToAdd;
        QSet<PackageKitResource*> m_packagesToDelete;

        struct Packages {
            QHash<QString, AbstractResource*> packages;
            QHash<QString, QStringList> packageToApp;
            QHash<QString, QVector<AppPackageKitResource*>> extendedBy;
            void clear() { *this = {}; }
        };

        QTimer m_delayedDetailsFetch;
        QSet<QString> m_packageNamesToFetchDetails;
        Packages m_packages;
        QSharedPointer<OdrsReviewsBackend> m_reviews;
};

#endif // PACKAGEKITBACKEND_H
