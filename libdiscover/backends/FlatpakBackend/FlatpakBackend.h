/***************************************************************************
 *   Copyright © 2013 Aleix Pol Gonzalez <aleixpol@blue-systems.com>       *
 *   Copyright © 2017 Jan Grulich <jgrulich@redhat.com>                    *
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

#ifndef FLATPAKBACKEND_H
#define FLATPAKBACKEND_H

#include "FlatpakResource.h"

#include <resources/AbstractResourcesBackend.h>
#include <QVariantList>
#include <QSharedPointer>
#include <QThreadPool>

#include <AppStreamQt/component.h>

extern "C" {
#include <flatpak.h>
}

class QAction;
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
    AbstractBackendUpdater * backendUpdater() const override;
    AbstractReviewsBackend * reviewsBackend() const override;
    ResultsStream * search(const AbstractResourcesBackend::Filters & search) override;
    ResultsStream * findResourceByPackageName(const QUrl &search);
    QList<FlatpakResource*> resources() const { return m_resources.values(); }
    bool isValid() const override;

    Transaction* installApplication(AbstractResource* app) override;
    Transaction* installApplication(AbstractResource* app, const AddonList& addons) override;
    Transaction* removeApplication(AbstractResource* app) override;
    bool isFetching() const override { return m_fetching; }
    AbstractResource * resourceForFile(const QUrl & ) override;
    void checkForUpdates() override;
    QString displayName() const override;
    bool hasApplications() const override { return true; }
    FlatpakResource * addSourceFromFlatpakRepo(const QUrl &url);

private Q_SLOTS:
    void onFetchMetadataFinished(FlatpakInstallation *flatpakInstallation, FlatpakResource *resource, const QByteArray &metadata);
    void onFetchSizeFinished(FlatpakResource *resource, guint64 downloadSize, guint64 installedSize);
    void onFetchUpdatesFinished(FlatpakInstallation *flatpakInstallation, GPtrArray *updates);

Q_SIGNALS: //for tests
    void initialized();

private:
    bool flatpakResourceLessThan(AbstractResource* l, AbstractResource* r);
    void announceRatingsReady();
    FlatpakInstallation * preferredInstallation() const { return m_installations.constFirst(); }
    void integrateRemote(FlatpakInstallation *flatpakInstallation, FlatpakRemote *remote);
    FlatpakRemote * getFlatpakRemoteByUrl(const QString &url, FlatpakInstallation *installation) const;
    FlatpakInstalledRef * getInstalledRefForApp(FlatpakInstallation *flatpakInstallation, FlatpakResource *resource) const;
    FlatpakResource * getAppForInstalledRef(FlatpakInstallation *flatpakInstallation, FlatpakInstalledRef *ref) const;
    FlatpakResource * getRuntimeForApp(FlatpakResource *resource) const;

    FlatpakResource * addAppFromFlatpakBundle(const QUrl &url);
    FlatpakResource * addAppFromFlatpakRef(const QUrl &url);
    void addResource(FlatpakResource *resource);
    bool compareAppFlatpakRef(FlatpakInstallation *flatpakInstallation, FlatpakResource *resource, FlatpakInstalledRef *ref) const;
    void finishInitialization();
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
    bool updateAppMetadata(FlatpakInstallation *flatpakInstallation, FlatpakResource *resource);
    bool updateAppMetadata(FlatpakResource *resource, const QByteArray &data);
    bool updateAppMetadata(FlatpakResource *resource, const QString &path);
    bool updateAppSize(FlatpakInstallation *flatpakInstallation, FlatpakResource *resource);
    bool updateAppSizeFromRemote(FlatpakInstallation *flatpakInstallation, FlatpakResource *resource);
    void updateAppState(FlatpakInstallation *flatpakInstallation, FlatpakResource *resource);

    void setFetching(bool fetching);

    QHash<QString, FlatpakResource*> m_resources;
    StandardBackendUpdater  *m_updater;
    FlatpakSourcesBackend *m_sources = nullptr;
    QSharedPointer<OdrsReviewsBackend> m_reviews;
    bool m_fetching;
    uint m_refreshAppstreamMetadataJobs;

    GCancellable *m_cancellable;
    QVector<FlatpakInstallation *> m_installations;
    QThreadPool m_threadPool;
};

#endif // FLATPAKBACKEND_H
