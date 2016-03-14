/***************************************************************************
 *   Copyright © 2012 Aleix Pol Gonzalez <aleixpol@blue-systems.com>       *
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

#ifndef KNSBACKEND_H
#define KNSBACKEND_H

// KDE includes
#include <KNewStuff3/kns3/entry.h>

// Attica includes
#include <attica/category.h>
#include <attica/provider.h>

// DiscoverCommon includes
#include <resources/AbstractResourcesBackend.h>
#include "Transaction/AddonList.h"

#include "discovercommon_export.h"

class KConfigGroup;
class KNSReviews;
namespace KNS3 { class DownloadManager; }
namespace Attica {
    class ProviderManager;
    class BaseJob;
}

class DISCOVERCOMMON_EXPORT KNSBackend : public AbstractResourcesBackend
{
Q_OBJECT
public:
    explicit KNSBackend(QObject* parent = nullptr);
    ~KNSBackend() override;
    
    void setMetaData(const QString& path) override;
    void cancelTransaction(AbstractResource* app) override;
    void removeApplication(AbstractResource* app) override;
    void installApplication(AbstractResource* app) override;
    void installApplication(AbstractResource* app, const AddonList& addons) override;
    AbstractResource* resourceByPackageName(const QString& name) const override;
    int updatesCount() const override;
    AbstractReviewsBackend* reviewsBackend() const override;
    QList<AbstractResource*> searchPackageName(const QString& searchText) override;
    QVector< AbstractResource* > allResources() const override;
    AbstractBackendUpdater* backendUpdater() const override;
    bool isFetching() const override;
    QList<QAction*> messageActions() const override { return QList<QAction*>(); }

    bool isValid() const override;
    Attica::Provider* provider() { return &m_provider; }
    QList<AbstractResource*> upgradeablePackages() const override;

public Q_SLOTS:
    void receivedEntries(const KNS3::Entry::List& entry);
    void startFetchingCategories();
    void categoriesLoaded(Attica::BaseJob*);
    void receivedContents(Attica::BaseJob*);
    void statusChanged(const KNS3::Entry& entry);

private:
    static void initManager(const QUrl& group);
    static QSharedPointer<Attica::ProviderManager> m_atticaManager;
    void setFetching(bool f);
    void markInvalid();
    
    bool m_fetching;
    bool m_isValid;
    KNS3::DownloadManager* m_manager;
    QHash<QString, AbstractResource*> m_resourcesByName;
    int m_page;
    KNSReviews* const m_reviews;
    Attica::Provider m_provider;
    QMap<QString, Attica::Category> m_categories;
    QString m_name;
    QString m_iconName;
    AbstractBackendUpdater* const m_updater;
};

#endif // KNSBACKEND_H
