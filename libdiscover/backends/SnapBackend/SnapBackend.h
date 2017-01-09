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
#include "SnapSocket.h"

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

    int updatesCount() const override;
    AbstractBackendUpdater* backendUpdater() const override;
    AbstractReviewsBackend* reviewsBackend() const override;
    bool isValid() const override { return true; } // No external file dependencies that could cause runtime errors
    QList<QAction*> messageActions() const override { return {}; }

    void installApplication(AbstractResource* app) override;
    void installApplication(AbstractResource* app, const AddonList& addons) override;
    void removeApplication(AbstractResource* app) override;
    bool isFetching() const override { return m_fetching; }
    SnapSocket* socket() { return &m_socket; }

private:
    void setFetching(bool fetching);
    ResultsStream* populate(SnapJob* snaps, AbstractResource::State state);

    QHash<QString, SnapResource*> m_resources;
    StandardBackendUpdater* m_updater;
    SnapReviewsBackend* m_reviews;

    SnapSocket m_socket;
    bool m_fetching = false;
};

#endif // SNAPBACKEND_H
