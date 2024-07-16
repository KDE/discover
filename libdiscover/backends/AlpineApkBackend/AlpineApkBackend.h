/*
 *   SPDX-FileCopyrightText: 2020 Alexey Minnekhanov <alexey.min@gmail.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#ifndef AlpineApkBackend_H
#define AlpineApkBackend_H

#include <resources/AbstractResourcesBackend.h>

#include <QFutureWatcher>
#include <QVariantList>

#include <QtApk>

#include <AppStreamQt/component-box.h>

class AlpineApkReviewsBackend;
class AlpineApkUpdater;
class AlpineApkResource;
class AppstreamDataDownloader;
class KJob;
class QTimer;

class AlpineApkBackend : public AbstractResourcesBackend
{
    Q_OBJECT

public:
    explicit AlpineApkBackend(QObject *parent = nullptr);

    QVector<Category *> category() const override;
    int updatesCount() const override;
    AbstractBackendUpdater *backendUpdater() const override;
    AbstractReviewsBackend *reviewsBackend() const override;
    ResultsStream *search(const AbstractResourcesBackend::Filters &filter) override;
    ResultsStream *findResourceByPackageName(const QUrl &search);
    QHash<QString, AlpineApkResource *> resources() const
    {
        return m_resources;
    }
    QHash<QString, AlpineApkResource *> *resourcesPtr()
    {
        return &m_resources;
    }
    bool isValid() const override
    {
        return true;
    } // No external file dependencies that could cause runtime errors

    Transaction *installApplication(AbstractResource *app) override;
    Transaction *installApplication(AbstractResource *app, const AddonList &addons) override;
    Transaction *removeApplication(AbstractResource *app) override;
    bool isFetching() const override
    {
        return m_fetching;
    }
    int fetchingUpdatesProgress() const override;
    void checkForUpdates() override;
    QString displayName() const override;
    bool hasApplications() const override;

public Q_SLOTS:
    void setFetchingUpdatesProgress(int percent);

private Q_SLOTS:
    void finishCheckForUpdates();
    void loadAppStreamComponents();
    void parseAppStreamMetadata();
    void reloadAppStreamMetadata();
    void fillResourcesAndApplyAppStreamData();
    void loadResources();
    void onLoadResourcesFinished();
    void onAppstreamDataDownloaded();

public:
    QtApk::Database *apkdb()
    {
        return &m_apkdb;
    }

private:
    QHash<QString, AlpineApkResource *> m_resources;
    QHash<QString, AppStream::Component> m_resourcesAppstreamData;
    AlpineApkUpdater *m_updater;
    AlpineApkReviewsBackend *m_reviews;
    QtApk::Database m_apkdb;
    QVector<QtApk::Package> m_availablePackages;
    QVector<QtApk::Package> m_installedPackages;
    bool m_fetching = false;
    int m_fetchProgress = 0;
    QTimer *m_updatesTimeoutTimer;
    AppStream::ComponentBox m_appStreamComponents;
    // QVector<QString> m_collectedCategories;
    QFutureWatcher<void> m_voidFutureWatcher;
    AppstreamDataDownloader *m_appstreamDownloader;
};

#endif // AlpineApkBackend_H
