/*
 *   SPDX-FileCopyrightText: 2012 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include <KNSCore/EntryInternal>
#include <KNSCore/ErrorCode>

#include "Transaction/AddonList.h"
#include "discovercommon_export.h"
#include <resources/AbstractResourcesBackend.h>

class KNSReviews;
class KNSResource;
class StandardBackendUpdater;

namespace KNSCore
{
class Engine;
}

class DISCOVERCOMMON_EXPORT KNSBackend : public AbstractResourcesBackend
{
    Q_OBJECT
public:
    explicit KNSBackend(QObject *parent, const QString &iconName, const QString &knsrc);
    ~KNSBackend() override;

    Transaction *removeApplication(AbstractResource *app) override;
    Transaction *installApplication(AbstractResource *app) override;
    Transaction *installApplication(AbstractResource *app, const AddonList &addons) override;
    int updatesCount() const override;
    AbstractReviewsBackend *reviewsBackend() const override;
    AbstractBackendUpdater *backendUpdater() const override;
    bool isFetching() const override;
    ResultsStream *search(const AbstractResourcesBackend::Filters &filter) override;
    ResultsStream *findResourceByPackageName(const QUrl &search);

    QVector<Category *> category() const override
    {
        return m_rootCategories;
    }
    bool hasApplications() const override
    {
        return m_hasApplications;
    }

    bool isValid() const override;

    QStringList extends() const override
    {
        return m_extends;
    }

    QString iconName() const
    {
        return m_iconName;
    }

    KNSCore::Engine *engine() const
    {
        return m_engine;
    }

    void checkForUpdates() override;

    QString displayName() const override;

Q_SIGNALS:
    void receivedResources(const QVector<AbstractResource *> &resources);
    void searchFinished();
    void startingSearch();
    void availableForQueries();
    void initialized();

public Q_SLOTS:
    void receivedEntries(const KNSCore::EntryInternal::List &entries);
    void statusChanged(const KNSCore::EntryInternal &entry);
    void detailsLoaded(const KNSCore::EntryInternal &entry);
    void slotErrorCode(const KNSCore::ErrorCode &errorCode, const QString &message, const QVariant &metadata);
    void slotEntryEvent(const KNSCore::EntryInternal &entry, KNSCore::EntryInternal::EntryEvent event);

private:
    void fetchInstalled();
    KNSResource *resourceForEntry(const KNSCore::EntryInternal &entry);
    void setFetching(bool f);
    void markInvalid(const QString &message);
    ResultsStream *searchStream(const QString &searchText);
    void fetchMore();
    void setResponsePending(bool pending);

    bool m_onePage = false;
    bool m_responsePending = false;
    QString m_pendingSearchQuery;
    bool m_fetching;
    bool m_isValid;
    KNSCore::Engine *m_engine;
    QHash<QString, AbstractResource *> m_resourcesByName;
    KNSReviews *const m_reviews;
    QString m_name;
    const QString m_iconName;
    StandardBackendUpdater *const m_updater;
    QStringList m_extends;
    QStringList m_categories;
    QVector<Category *> m_rootCategories;
    QString m_displayName;
    bool m_initialized = false;
    bool m_hasApplications = false;
};
