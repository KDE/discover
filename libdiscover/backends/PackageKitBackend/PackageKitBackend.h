/*
 *   SPDX-FileCopyrightText: 2012 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#ifndef PACKAGEKITBACKEND_H
#define PACKAGEKITBACKEND_H

#include "PackageKitResource.h"
#include <AppStreamQt/pool.h>
#include <PackageKit/Transaction>
#include <QFile>
#include <QPointer>
#include <QSet>
#include <QSharedPointer>
#include <QStringList>
#include <QThreadPool>
#include <QTimer>
#include <QVariantList>
#include <resources/AbstractResourcesBackend.h>

class AppPackageKitResource;
class PackageKitUpdater;
class OdrsReviewsBackend;
class PKResultsStream;
class PKResolveTransaction;

class DISCOVERCOMMON_EXPORT PackageKitBackend : public AbstractResourcesBackend
{
    Q_OBJECT
public:
    explicit PackageKitBackend(QObject *parent = nullptr);
    ~PackageKitBackend() override;

    AbstractBackendUpdater *backendUpdater() const override;
    AbstractReviewsBackend *reviewsBackend() const override;
    QSet<AbstractResource *> resourcesByPackageName(const QString &name) const;

    ResultsStream *search(const AbstractResourcesBackend::Filters &search) override;
    PKResultsStream *findResourceByPackageName(const QUrl &search);
    int updatesCount() const override;
    bool hasSecurityUpdates() const override;

    Transaction *installApplication(AbstractResource *app) override;
    Transaction *installApplication(AbstractResource *app, const AddonList &addons) override;
    Transaction *removeApplication(AbstractResource *app) override;
    bool isValid() const override
    {
        return !QFile::exists(QStringLiteral("/run/ostree-booted"));
    }
    QSet<AbstractResource *> upgradeablePackages() const;
    bool isFetching() const override;

    bool isPackageNameUpgradeable(const PackageKitResource *res) const;
    QSet<QString> upgradeablePackageId(const PackageKitResource *res) const;
    QVector<AppPackageKitResource *> extendedBy(const QString &id) const;

    void resolvePackages(const QStringList &packageNames);
    void fetchDetails(const QString &pkgid)
    {
        fetchDetails(QSet<QString>{pkgid});
    }
    void fetchDetails(const QSet<QString> &pkgid);

    void checkForUpdates() override;
    QString displayName() const override;

    bool hasApplications() const override
    {
        return true;
    }
    static QString locateService(const QString &filename);

    QList<AppStream::Component> componentsById(const QString &id) const;
    void fetchUpdates();
    int fetchingUpdatesProgress() const override;

    HelpfulError *explainDysfunction() const override;

    void addPackageArch(PackageKit::Transaction::Info info, const QString &packageId, const QString &summary);
    void addPackageNotArch(PackageKit::Transaction::Info info, const QString &packageId, const QString &summary);

public Q_SLOTS:
    void reloadPackageList();
    void transactionError(PackageKit::Transaction::Error, const QString &message);

private Q_SLOTS:
    void getPackagesFinished();
    void addPackage(PackageKit::Transaction::Info info, const QString &packageId, const QString &summary, bool arch);
    void packageDetails(const PackageKit::Details &details);
    void addPackageToUpdate(PackageKit::Transaction::Info, const QString &pkgid, const QString &summary);
    void getUpdatesFinished(PackageKit::Transaction::Exit, uint);

Q_SIGNALS:
    void loadedAppStream();
    void available();

private:
    friend class PackageKitResource;
    template<typename T, typename W>
    T resourcesByPackageNames(const W &names) const;

    template<typename T>
    T resourcesByComponents(const QList<AppStream::Component> &names) const;

    void runWhenInitialized(const std::function<void()> &f, QObject *stream);

    void checkDaemonRunning();
    void acquireFetching(bool f);
    void includePackagesToAdd();
    void performDetailsFetch();
    AppPackageKitResource *addComponent(const AppStream::Component &component);
    void updateProxy();

    QScopedPointer<AppStream::Pool> m_appdata;
    PackageKitUpdater *m_updater;
    QPointer<PackageKit::Transaction> m_refresher;
    int m_isFetching;
    QSet<QString> m_updatesPackageId;
    bool m_hasSecurityUpdates = false;
    QSet<PackageKitResource *> m_packagesToAdd;
    QSet<PackageKitResource *> m_packagesToDelete;
    bool m_appstreamInitialized = false;

    struct {
        QHash<QString, AbstractResource *> packages;
        QHash<QString, QStringList> packageToApp;
        QHash<QString, QVector<AppPackageKitResource *>> extendedBy;
    } m_packages;

    QTimer m_delayedDetailsFetch;
    QSet<QString> m_packageNamesToFetchDetails;
    QSharedPointer<OdrsReviewsBackend> m_reviews;
    QPointer<PackageKit::Transaction> m_getUpdatesTransaction;
    QThreadPool m_threadPool;
    QPointer<PKResolveTransaction> m_resolveTransaction;
};

#endif // PACKAGEKITBACKEND_H
