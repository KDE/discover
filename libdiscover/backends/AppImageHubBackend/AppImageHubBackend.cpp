#include "AppImageHubBackend.h"
#include "AppImageHubResource.h"
#include "AppImageHubTransaction.h"
#include "libdiscover_backend_appimagehub_debug.h"

#include <KLocalizedString>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QNetworkReply>
#include <resources/StandardBackendUpdater.h>

DISCOVER_BACKEND_PLUGIN(AppImageHubBackend)

AppImageHubBackend::AppImageHubBackend(QObject *parent)
    : AbstractResourcesBackend(parent)
    , m_updater(new StandardBackendUpdater(this))
{
    qCDebug(LIBDISCOVER_BACKEND_APPIMAGEHUB_LOG) << "Creating AppImageHub backend";
    loadFeed();
}

void AppImageHubBackend::loadFeed()
{
    const QUrl feedUrl(QStringLiteral("https://appimage.github.io/feed.json"));
    qCDebug(LIBDISCOVER_BACKEND_APPIMAGEHUB_LOG) << "Requesting catalog" << feedUrl;
    auto *reply = m_manager.get(QNetworkRequest(feedUrl));
    connect(reply, &QNetworkReply::finished, this, [this, reply] {
        qCDebug(LIBDISCOVER_BACKEND_APPIMAGEHUB_LOG) << "Catalog request finished"
                                                       << "error=" << reply->errorString()
                                                       << "status=" << reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt()
                                                       << "bytes=" << reply->bytesAvailable();
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            qCWarning(LIBDISCOVER_BACKEND_APPIMAGEHUB_LOG) << "Catalog request failed:" << reply->errorString();
            Q_EMIT passiveMessage(i18n("Could not load the AppImageHub catalog: %1", reply->errorString()));
            m_loaded = true;
            finishPendingSearches();
            Q_EMIT fetchingUpdatesProgressChanged();
            return;
        }
        qDeleteAll(m_resources);
        m_resources.clear();
        QJsonParseError parseError;
        const auto document = QJsonDocument::fromJson(reply->readAll(), &parseError);
        if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
            qCWarning(LIBDISCOVER_BACKEND_APPIMAGEHUB_LOG) << "Invalid catalog JSON:" << parseError.errorString();
            m_loaded = true;
            finishPendingSearches();
            Q_EMIT fetchingUpdatesProgressChanged();
            return;
        }
        const auto items = document.object().value(QStringLiteral("items")).toArray();
        qCDebug(LIBDISCOVER_BACKEND_APPIMAGEHUB_LOG) << "Catalog contains" << items.size() << "items";
        for (const auto &item : items) {
            auto *resource = new AppImageHubResource(item.toObject(), this);
            if (resource->hasDownloadUrl()) {
                m_resources.append(resource);
            } else {
                qCDebug(LIBDISCOVER_BACKEND_APPIMAGEHUB_LOG) << "Skipping resource without a download URL" << resource->name();
                resource->deleteLater();
            }
        }
        m_loaded = true;
        qCDebug(LIBDISCOVER_BACKEND_APPIMAGEHUB_LOG) << "Loaded" << m_resources.size() << "resources";
        finishPendingSearches();
        Q_EMIT contentsChanged();
        Q_EMIT fetchingUpdatesProgressChanged();
    });
}

ResultsStream *AppImageHubBackend::search(const Filters &filter)
{
    if (!m_loaded) {
        auto *stream = new ResultsStream(QStringLiteral("AppImageHubSearch"));
        qCDebug(LIBDISCOVER_BACKEND_APPIMAGEHUB_LOG) << "Deferring search until catalog is loaded";
        m_pendingSearches.append({stream, filter});
        connect(stream, &QObject::destroyed, this, [this, stream] {
            m_pendingSearches.removeIf([stream](const PendingSearch &pending) {
                return pending.stream == stream;
            });
        });
        return stream;
    }

    const auto results = resultsForFilter(filter);
    return new ResultsStream(QStringLiteral("AppImageHubSearch"), results);
}

QVector<StreamResult> AppImageHubBackend::resultsForFilter(const Filters &filter) const
{
    QVector<StreamResult> results;
    qCDebug(LIBDISCOVER_BACKEND_APPIMAGEHUB_LOG) << "Search filters: text=" << filter.search << "state=" << filter.state
                                                   << "mimetype=" << filter.mimetype << "origin=" << filter.origin
                                                   << "extends=" << filter.extends << "resourceUrl=" << filter.resourceUrl
                                                   << "category=" << bool(filter.category);
    for (auto *resource : std::as_const(m_resources)) {
        if (!filter.search.isEmpty() && !resource->name().contains(filter.search, Qt::CaseInsensitive)
            && !resource->comment().contains(filter.search, Qt::CaseInsensitive))
            continue;
        const auto appstreamUrl = QUrl(QStringLiteral("appstream://") + resource->packageName());
        if (!filter.resourceUrl.isEmpty() && filter.resourceUrl != resource->url() && filter.resourceUrl != appstreamUrl)
            continue;
        if (filter.state != AbstractResource::Broken && resource->state() < filter.state)
            continue;
        results.append(resource);
    }
    if (filter.isEmpty()) {
        qCDebug(LIBDISCOVER_BACKEND_APPIMAGEHUB_LOG) << "Empty filter, returning" << results.size() << "resources";
        return results;
    }
    // Broken is the default sentinel meaning "any application", not an exact state filter.
    auto additionalFilter = filter;
    if (additionalFilter.state == AbstractResource::Broken) {
        additionalFilter.state = AbstractResource::None;
        additionalFilter.filterMinimumState = true;
    }
    additionalFilter.filterJustInCase(results);
    qCDebug(LIBDISCOVER_BACKEND_APPIMAGEHUB_LOG) << "Search" << filter.search << "returned" << results.size() << "resources";
    return results;
}

void AppImageHubBackend::finishPendingSearches()
{
    const auto pendingSearches = std::exchange(m_pendingSearches, {});
    for (const auto &pending : pendingSearches) {
        if (!pending.stream)
            continue;
        const auto results = resultsForFilter(pending.filter);
        if (!results.isEmpty())
            Q_EMIT pending.stream->resourcesFound(results);
        pending.stream->finish();
    }
}

AbstractBackendUpdater *AppImageHubBackend::backendUpdater() const
{
    return m_updater;
}

Transaction *AppImageHubBackend::installApplication(AbstractResource *app, const AddonList &addons)
{
    Q_UNUSED(addons)
    return new AppImageHubTransaction(qobject_cast<AppImageHubResource *>(app), Transaction::InstallRole);
}

Transaction *AppImageHubBackend::removeApplication(AbstractResource *app)
{
    return new AppImageHubTransaction(qobject_cast<AppImageHubResource *>(app), Transaction::RemoveRole);
}

void AppImageHubBackend::checkForUpdates()
{
    loadFeed();
}

#include "AppImageHubBackend.moc"
#include "moc_AppImageHubBackend.cpp"