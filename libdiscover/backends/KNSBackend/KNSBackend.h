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

#include <KNSCore/EntryInternal>

#include <resources/AbstractResourcesBackend.h>
#include "Transaction/AddonList.h"
#include "discovercommon_export.h"

class KConfigGroup;
class KNSReviews;
class KNSResource;
class StandardBackendUpdater;

namespace KNSCore { class Engine; }

class DISCOVERCOMMON_EXPORT KNSBackend : public AbstractResourcesBackend
{
Q_OBJECT
public:
    explicit KNSBackend(QObject* parent, const QString& iconName, const QString &knsrc);
    ~KNSBackend() override;

    Transaction* removeApplication(AbstractResource* app) override;
    Transaction* installApplication(AbstractResource* app) override;
    Transaction* installApplication(AbstractResource* app, const AddonList& addons) override;
    int updatesCount() const override;
    AbstractReviewsBackend* reviewsBackend() const override;
    AbstractBackendUpdater* backendUpdater() const override;
    bool isFetching() const override;
    ResultsStream* search(const AbstractResourcesBackend::Filters & filter) override;
    ResultsStream* findResourceByPackageName(const QUrl & search);

    QVector<Category*> category() const override { return m_rootCategories; }

    bool isValid() const override;

    QStringList extends() const override { return m_extends; }

    QString iconName() const { return m_iconName; }

    KNSCore::Engine* engine() const { return m_engine; }

    void checkForUpdates() override {}

    QString displayName() const override;

Q_SIGNALS:
    void receivedResources(const QVector<AbstractResource*> &resources);
    void searchFinished();
    void startingSearch();
    void availableForQueries();
    void initialized();

public Q_SLOTS:
    void receivedEntries(const KNSCore::EntryInternal::List& entries);
    void statusChanged(const KNSCore::EntryInternal& entry);

private:
    void fetchInstalled();
    KNSResource* resourceForEntry(const KNSCore::EntryInternal& entry);
    void setFetching(bool f);
    void markInvalid(const QString &message);
    void searchStream(ResultsStream* stream, const QString &searchText);
    
    bool m_onePage = false;
    bool m_responsePending = false;
    bool m_fetching;
    bool m_isValid;
    KNSCore::Engine* m_engine;
    QHash<QString, AbstractResource*> m_resourcesByName;
    KNSReviews* const m_reviews;
    QString m_name;
    QString m_iconName;
    StandardBackendUpdater* const m_updater;
    QStringList m_extends;
    QStringList m_categories;
    QVector<Category*> m_rootCategories;
    QString m_displayName;
};

#endif // KNSBACKEND_H
