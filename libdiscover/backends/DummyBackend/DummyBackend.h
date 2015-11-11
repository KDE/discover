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

#ifndef DUMMYBACKEND_H
#define DUMMYBACKEND_H

#include <resources/AbstractResourcesBackend.h>
#include <QVariantList>

class QAction;
class DummyReviewsBackend;
class StandardBackendUpdater;
class DummyResource;
class DummyBackend : public AbstractResourcesBackend
{
Q_OBJECT
Q_PROPERTY(int startElements MEMBER m_startElements)
public:
    explicit DummyBackend(QObject* parent = nullptr);

    virtual void setMetaData(const QString& path) override;
    virtual QList<AbstractResource*> upgradeablePackages() const override;
    virtual AbstractResource* resourceByPackageName(const QString& name) const override;
    virtual int updatesCount() const override;
    virtual AbstractBackendUpdater* backendUpdater() const override;
    virtual AbstractReviewsBackend* reviewsBackend() const override;
    virtual QList<AbstractResource*> searchPackageName(const QString& searchText) override;
    virtual QVector<AbstractResource*> allResources() const override;
    virtual bool isValid() const override { return true; } // No external file dependencies that could cause runtime errors
    virtual QList<QAction*> messageActions() const override { return m_messageActions; }

    virtual void cancelTransaction(AbstractResource* app) override;
    virtual void installApplication(AbstractResource* app) override;
    virtual void installApplication(AbstractResource* app, AddonList addons) override;
    virtual void removeApplication(AbstractResource* app) override;
    virtual bool isFetching() const override { return m_fetching; }

public Q_SLOTS:
    void checkForUpdates();
    void toggleFetching();

private:
    void populate(const QString& name);

    QHash<QString, DummyResource*> m_resources;
    StandardBackendUpdater* m_updater;
    DummyReviewsBackend* m_reviews;
    bool m_fetching;
    int m_startElements;
    QList<QAction*> m_messageActions;
};

#endif // DUMMYBACKEND_H
