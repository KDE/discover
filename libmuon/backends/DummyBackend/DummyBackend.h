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

class DummyResource;
class DummyBackend : public AbstractResourcesBackend
{
Q_OBJECT
Q_INTERFACES(AbstractResourcesBackend)
public:
    explicit DummyBackend(QObject* parent, const QVariantList& args);

    virtual QList<AbstractResource*> upgradeablePackages() const;
    virtual AbstractResource* resourceByPackageName(const QString& name) const;
    virtual int updatesCount() const;
    virtual AbstractBackendUpdater* backendUpdater() const;
    virtual AbstractReviewsBackend* reviewsBackend() const;
    virtual QList<AbstractResource*> searchPackageName(const QString& searchText);
    virtual QVector<AbstractResource*> allResources() const;
    virtual bool isValid() const { return true; } // No external file dependencies that could cause runtime errors

    virtual void cancelTransaction(AbstractResource* app);
    virtual void installApplication(AbstractResource* app);
    virtual void installApplication(AbstractResource* app, AddonList addons);
    virtual void removeApplication(AbstractResource* app);
    virtual bool isFetching() const { return false; }

private:
    QHash<QString, DummyResource*> m_resources;
    AbstractBackendUpdater* m_updater;
    AbstractReviewsBackend* m_reviews;
};

#endif // DUMMYBACKEND_H
