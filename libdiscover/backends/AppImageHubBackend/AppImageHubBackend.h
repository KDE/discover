#pragma once

#include <QList>
#include <QNetworkAccessManager>
#include <QPointer>
#include <resources/AbstractResourcesBackend.h>

class StandardBackendUpdater;
class AppImageHubResource;
class AppImageHubBackend : public AbstractResourcesBackend
{
    Q_OBJECT
public:
    explicit AppImageHubBackend(QObject *parent = nullptr);
    using AbstractResourcesBackend::installApplication;
    bool isValid() const override { return true; }
    ResultsStream *search(const Filters &filter) override;
    AbstractReviewsBackend *reviewsBackend() const override { return nullptr; }
    AbstractBackendUpdater *backendUpdater() const override;
    int updatesCount() const override { return 0; }
    int fetchingUpdatesProgress() const override { return m_loaded ? 100 : 0; }
    bool hasApplications() const override { return true; }
    QString displayName() const override { return QStringLiteral("AppImageHub"); }
    Transaction *installApplication(AbstractResource *app, const AddonList &addons) override;
    Transaction *removeApplication(AbstractResource *app) override;
    void checkForUpdates() override;

private:
    void loadFeed();
    QVector<StreamResult> resultsForFilter(const Filters &filter) const;
    void finishPendingSearches();

    struct PendingSearch {
        QPointer<ResultsStream> stream;
        Filters filter;
    };

    QVector<AppImageHubResource *> m_resources;
    QList<PendingSearch> m_pendingSearches;
    QNetworkAccessManager m_manager;
    StandardBackendUpdater *m_updater;
    bool m_loaded = false;
};