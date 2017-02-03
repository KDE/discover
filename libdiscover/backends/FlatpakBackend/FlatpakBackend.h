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

#include <resources/AbstractResourcesBackend.h>
#include <QVariantList>

extern "C" {
#include <libappstream-glib/appstream-glib.h>
#include <flatpak.h>
#include <gio/gio.h>
#include <glib.h>
}

class QAction;
class FlatpakReviewsBackend;
class StandardBackendUpdater;
class FlatpakResource;
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
    ResultsStream * findResourceByPackageName(const QUrl &search) override;
    QHash<QString, FlatpakResource*> resources() const { return m_resources; }
    bool isValid() const override { return true; } // No external file dependencies that could cause runtime errors
    QList<QAction*> messageActions() const override { return m_messageActions; }

    FlatpakInstallation *flatpakInstallationForAppScope(AsAppScope appScope) const;
    void installApplication(AbstractResource* app) override;
    void installApplication(AbstractResource* app, const AddonList& addons) override;
    void removeApplication(AbstractResource* app) override;
    bool isFetching() const override { return m_fetching; }
//     AbstractResource * resourceForFile(const QUrl & ) override;

public Q_SLOTS:
    void checkForUpdates();

private:
    FlatpakRef * createFakeRef(FlatpakResource *resource);
    FlatpakInstalledRef * getInstalledRefForApp(FlatpakInstallation *flatpakInstallation, FlatpakResource *resource, GCancellable *cancellable);
    FlatpakResource * getRuntimeForApp(FlatpakResource *resource);
    bool compareAppFlatpakRef(FlatpakInstallation *flatpakInstallation, FlatpakResource *resource, FlatpakInstalledRef *ref);
    bool loadAppsFromAppstreamData(FlatpakInstallation *flatpakInstallation, GCancellable *cancellable);
    bool loadInstalledApps(FlatpakInstallation *flatpakInstallation, GCancellable *cancellable);
    bool parseMetadataFromAppBundle(FlatpakResource *resource);
    void reloadPackageList(GCancellable *cancellable);
    bool setupFlatpakInstallations(GCancellable *cancellable, GError **error);
    void updateAppInstalledMetadata(FlatpakInstalledRef *installedRef, FlatpakResource *resource);
    bool updateAppMetadata(FlatpakInstallation *flatpakInstallation, FlatpakResource *resource, GCancellable *cancellable);
    bool updateAppSize(FlatpakInstallation *flatpakInstallation, FlatpakResource *resource, GCancellable *cancellable);
    void updateAppState(FlatpakInstallation *flatpakInstallation, FlatpakResource *resource, GCancellable *cancellable);

    QHash<QString, FlatpakResource*> m_resources;
    StandardBackendUpdater  *m_updater;
    FlatpakReviewsBackend *m_reviews;
    bool m_fetching;
    QList<QAction*> m_messageActions;

    AsStore *m_store;
    FlatpakInstallation *m_flatpakInstallationUser = nullptr;
    FlatpakInstallation *m_flatpakInstallationSystem = nullptr;
};

#endif // FLATPAKBACKEND_H
