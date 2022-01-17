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
class FlatpakSource;
class StandardBackendUpdater;
class OdrsReviewsBackend;

namespace AppStream
{
class Pool;
}

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
    FlatpakInstalledRef *getInstalledRefForApp(const FlatpakResource *resource) const;
    void loadRemote(FlatpakInstallation *installation, FlatpakRemote *remote);
    void unloadRemote(FlatpakInstallation *installation, FlatpakRemote *remote);

    HelpfulError *explainDysfunction() const override;
    QVector<FlatpakInstallation *> installations() const
    {
        return m_installations;
    }

private Q_SLOTS:
    void onFetchMetadataFinished(FlatpakResource *resource, const QByteArray &metadata);
    void onFetchSizeFinished(FlatpakResource *resource, guint64 downloadSize, guint64 installedSize);

Q_SIGNALS: // for tests
    void initialized();

private:
    friend class FlatpakSource;

    void metadataRefreshed();
    bool flatpakResourceLessThan(AbstractResource *l, AbstractResource *r) const;
    void announceRatingsReady();
    FlatpakInstallation *preferredInstallation() const
    {
        return m_installations.constFirst();
    }
    QSharedPointer<FlatpakSource> integrateRemote(FlatpakInstallation *flatpakInstallation, FlatpakRemote *remote);
    FlatpakRemote *getFlatpakRemoteByUrl(const QString &url, FlatpakInstallation *installation) const;
    FlatpakResource *getRuntimeForApp(FlatpakResource *resource) const;
    FlatpakResource *resourceForComponent(const AppStream::Component &component, const QSharedPointer<FlatpakSource> &source) const;

    void loadAppsFromAppstreamData();
    bool loadAppsFromAppstreamData(FlatpakInstallation *flatpakInstallation);
    void loadLocalUpdates(FlatpakInstallation *flatpakInstallation);
    bool parseMetadataFromAppBundle(FlatpakResource *resource);
    bool setupFlatpakInstallations(GError **error);
    void updateAppInstalledMetadata(FlatpakInstalledRef *installedRef, FlatpakResource *resource);
    bool updateAppMetadata(FlatpakResource *resource);
    bool updateAppMetadata(FlatpakResource *resource, const QByteArray &data);
    bool updateAppMetadata(FlatpakResource *resource, const QString &path);
    bool updateAppSizeFromRemote(FlatpakResource *resource);
    void updateAppState(FlatpakResource *resource);
    QSharedPointer<FlatpakSource> findSource(FlatpakInstallation *installation, const QString &origin) const;

    QVector<AbstractResource *> resourcesByAppstreamName(const QString &name) const;
    void acquireFetching(bool f);
    void checkForUpdates(FlatpakInstallation *flatpakInstallation, FlatpakRemote *remote);

    StandardBackendUpdater *m_updater;
    FlatpakSourcesBackend *m_sources = nullptr;
    QSharedPointer<OdrsReviewsBackend> m_reviews;
    uint m_isFetching = 0;
    uint m_refreshAppstreamMetadataJobs;
    QStringList m_extends;

    GCancellable *m_cancellable;
    QVector<FlatpakInstallation *> m_installations;
    QThreadPool m_threadPool;
    QVector<QSharedPointer<FlatpakSource>> m_flatpakSources;
    QVector<QSharedPointer<FlatpakSource>> m_flatpakLoadingSources;
    QSharedPointer<FlatpakSource> m_localSource;
};

#endif // FLATPAKBACKEND_H
