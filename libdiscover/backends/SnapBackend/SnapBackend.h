/***************************************************************************
 *   Copyright © 2013 Aleix Pol Gonzalez <aleixpol@blue-systems.com>       *
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

#ifndef SNAPBACKEND_H
#define SNAPBACKEND_H

#include <resources/AbstractResource.h>
#include <resources/AbstractResourcesBackend.h>
#include <QVariantList>
#include <Snapd/Client>

class QAction;
class SnapReviewsBackend;
class StandardBackendUpdater;
class SnapResource;
class SnapBackend : public AbstractResourcesBackend
{
Q_OBJECT
public:
    explicit SnapBackend(QObject* parent = nullptr);

    ResultsStream * search(const AbstractResourcesBackend::Filters & search) override;
    ResultsStream * findResourceByPackageName(const QUrl& search) override;

    QString displayName() const override;
    int updatesCount() const override;
    AbstractBackendUpdater* backendUpdater() const override;
    AbstractReviewsBackend* reviewsBackend() const override;
    bool isValid() const override { return m_valid; }

    Transaction* installApplication(AbstractResource* app) override;
    Transaction* installApplication(AbstractResource* app, const AddonList& addons) override;
    Transaction* removeApplication(AbstractResource* app) override;
    bool isFetching() const override { return m_fetching; }
    void checkForUpdates() override {}
    bool hasApplications() const override { return true; }
    QSnapdClient* client() { return &m_client; }
    void refreshStates();

private:
    void setFetching(bool fetching);
    template <class T>
    ResultsStream* populate(T* snaps);

    QHash<QString, SnapResource*> m_resources;
    StandardBackendUpdater* m_updater;
    SnapReviewsBackend* m_reviews;

    bool m_valid = true;
    bool m_fetching = false;
    QSnapdClient m_client;
};

#endif // SNAPBACKEND_H
