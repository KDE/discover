/*
 *   SPDX-FileCopyrightText: 2013 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *   SPDX-FileCopyrightText: 2017 Jan Grulich <jgrulich@redhat.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#ifndef FLATPAKBACKEND_H
#define FLATPAKBACKEND_H

#include "FlatpakResource.h"

#include <QSharedPointer>
#include <QThreadPool>
#include <QVariantList>
#include <resources/AbstractResourcesBackend.h>

#include <AppStreamQt/component.h>

#include "flatpak-helper.h"

class FlatpakSourcesBackend;
class StandardBackendUpdater;
class OdrsReviewsBackend;
class FlatpakBackend : public AbstractResourcesBackend
{
    Q_OBJECT
public:
    explicit FlatpakBackend(QObject *parent = nullptr);
    ~FlatpakBackend();

    int updatesCount() const override;
    AbstractBackendUpdater *backendUpdater() const override;
    AbstractReviewsBackend *reviewsBackend() const override;
    ResultsStream *search(const AbstractResourcesBackend::Filters &search) override;
    ResultsStream *findResourceByPackageName(const QUrl &search);
    QList<FlatpakResource *> resources() const
    {
        return m_resources.values();
    }
    bool isValid() const override;

    Transaction *installApplication(AbstractResource *app) override;
    Transaction *installApplication(AbstractResource *app, const AddonList &addons) override;
    Transaction *removeApplication(AbstractResource *app) override;
    bool isFetching() const override
    {
        return m_isFetching > 0;
    }
    void checkForUpdates() override;
    QString displayName() const override;
    bool hasApplications() const override
    {
        return true;
    }
    QStringList extends() const override
    {
        return m_extends;
    }

    void addSourceFromFlatpakRepo(const QUrl &url, ResultsStream *stream);
    void addAppFromFlatpakBundle(const QUrl &url, ResultsStream *stream);
    void addAppFromFlatpakRef(const QUrl &url, ResultsStream *stream);
    FlatpakResource *getAppForInstalledRef(FlatpakInstallation *flatpakInstallation, FlatpakInstalledRef *ref) const;

    FlatpakSourcesBackend *sources() const
    {
        return m_sources;
    }

    bool updateAppSize(FlatpakResource *resource);

private Q_SLOTS:
    void onFetchMetadataFinished(FlatpakResource *resource, const QByteArray &metadata);
    void onFetchSizeFinished(FlatpakResource *resource, guint64 downloadSize, guint64 installedSize);
    void onFetchUpdatesFinished(FlatpakInstallation *flatpakInstallation, GPtrArray *updates);

Q_SIGNALS: // for tests
    void initialized();

private:
    void metadataRefreshed();
    bool flatpakResourceLessThan(AbstractResource *l, AbstractResource *r) const;
    void announceRatingsReady();
    FlatpakInstallation *preferredInstallation() const
    {
        return m_installations.constFirst();
    }
    void integrateRemote(FlatpakInstallation *flatpakInstallation, FlatpakRemote *remote);
    FlatpakRemote *getFlatpakRemoteByUrl(const QString &url, FlatpakInstallation *installation) const;
    FlatpakInstalledRef *getInstalledRefForApp(FlatpakResource *resource) const;
    FlatpakResource *getRuntimeForApp(FlatpakResource *resource) const;

    void addResource(FlatpakResource *resource);
    void loadAppsFromAppstreamData();
    bool loadAppsFromAppstreamData(FlatpakInstallation *flatpakInstallation);
    void loadInstalledApps();
    bool loadInstalledApps(FlatpakInstallation *flatpakInstallation);
    void loadLocalUpdates(FlatpakInstallation *flatpakInstallation);
    void loadRemoteUpdates(FlatpakInstallation *flatpakInstallation);
    bool parseMetadataFromAppBundle(FlatpakResource *resource);
    void refreshAppstreamMetadata(FlatpakInstallation *installation, FlatpakRemote *remote);
    bool setupFlatpakInstallations(GError **error);
    void updateAppInstalledMetadata(FlatpakInstalledRef *installedRef, FlatpakResource *resource);
    bool updateAppMetadata(FlatpakResource *resource);
    bool updateAppMetadata(FlatpakResource *resource, const QByteArray &data);
    bool updateAppMetadata(FlatpakResource *resource, const QString &path);
    bool updateAppSizeFromRemote(FlatpakResource *resource);
    void updateAppState(FlatpakResource *resource);

    QVector<AbstractResource *> resourcesByAppstreamName(const QString &name) const;
    void acquireFetching(bool f);

    QHash<FlatpakResource::Id, FlatpakResource *> m_resources;
    StandardBackendUpdater *m_updater;
    FlatpakSourcesBackend *m_sources = nullptr;
    QSharedPointer<OdrsReviewsBackend> m_reviews;
    uint m_isFetching = 0;
    uint m_refreshAppstreamMetadataJobs;
    QStringList m_extends;

    GCancellable *m_cancellable;
    QVector<FlatpakInstallation *> m_installations;
    QThreadPool m_threadPool;
};

#endif // FLATPAKBACKEND_H
