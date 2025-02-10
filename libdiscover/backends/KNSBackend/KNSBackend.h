/*
 *   SPDX-FileCopyrightText: 2012 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include <KNSCore/EngineBase>
#include <KNSCore/Entry>
#include <KNSCore/ErrorCode>

#include "Transaction/AddonList.h"
#include "discovercommon_export.h"
#include <resources/AbstractResourcesBackend.h>

class KNSReviews;
class KNSResource;
class KNSResultsStream;
class StandardBackendUpdater;

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
    ResultsStream *search(const AbstractResourcesBackend::Filters &filter) override;
    ResultsStream *findResourceByPackageName(const QUrl &search);

    QList<std::shared_ptr<Category>> category() const override
    {
        return m_rootCategories;
    }
    bool hasApplications() const override
    {
        return m_hasApplications;
    }

    bool isValid() const override;

    bool extends(const QString &id) const override
    {
        return m_extends.contains(id);
    }

    QStringList extends() const
    {
        return m_extends;
    }

    QString iconName() const
    {
        return m_iconName;
    }

    KNSCore::EngineBase *engine() const
    {
        return m_engine;
    }

    void checkForUpdates() override;

    QString displayName() const override;
    KNSResource *resourceForEntry(const KNSCore::Entry &entry);

    int fetchingUpdatesProgress() const override
    {
        return m_fetching > 0 ? 42 : 100;
    }

Q_SIGNALS:
    void initialized();

public Q_SLOTS:
    void statusChanged(const KNSCore::Entry &entry);
    void detailsLoaded(const KNSCore::Entry &entry);
    void slotErrorCode(const KNSCore::ErrorCode::ErrorCode &errorCode, const QString &message, const QVariant &metadata);
    void slotEntryEvent(const KNSCore::Entry &entry, KNSCore::Entry::EntryEvent event);

private:
    void fetchInstalled();
    void setFetching(bool f);
    void markInvalid(const QString &message);

    template<typename T>
    void deferredResultStream(KNSResultsStream *stream, T start);
    KNSResultsStream *searchStream(const QString &searchText);

    bool m_fetching;
    bool m_isValid;
    KNSCore::EngineBase *m_engine;
    QHash<QString, AbstractResource *> m_resourcesByName;
    KNSReviews *const m_reviews;
    QString m_name;
    const QString m_iconName;
    StandardBackendUpdater *const m_updater;
    QStringList m_extends;
    QStringList m_categories;
    QList<std::shared_ptr<Category>> m_rootCategories;
    QString m_displayName;
    bool m_initialized = false;
    bool m_hasApplications = false;
};
