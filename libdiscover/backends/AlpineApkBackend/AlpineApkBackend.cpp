/***************************************************************************
 *   Copyright © 2020 Alexey Min <alexey.min@gmail.com>                    *
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

#include "AlpineApkBackend.h"
#include "AlpineApkResource.h"
#include "AlpineApkReviewsBackend.h"
#include "AlpineApkTransaction.h"
#include "AlpineApkSourcesBackend.h"
#include "AlpineApkUpdater.h"
#include "AppstreamDataDownloader.h"
#include "alpineapk_backend_logging.h"  // generated by ECM

#include "resources/SourcesModel.h"
#include "Transaction/Transaction.h"
#include "Category/Category.h"

#include <KLocalizedString>

#include <AppStreamQt/pool.h>
#include <AppStreamQt/version.h>

#include <QAction>
#include <QtConcurrentRun>
#include <QDebug>
#include <QFuture>
#include <QFutureWatcher>
#include <QLoggingCategory>
#include <QSet>
#include <QThread>
#include <QThreadPool>
#include <QTimer>

#include <utility>

DISCOVER_BACKEND_PLUGIN(AlpineApkBackend)

AlpineApkBackend::AlpineApkBackend(QObject *parent)
    : AbstractResourcesBackend(parent)
    , m_updater(new AlpineApkUpdater(this))
    , m_reviews(new AlpineApkReviewsBackend(this))
    , m_updatesTimeoutTimer(new QTimer(this))
    , m_appStreamComponents(AppStream::ComponentBox::Flag::FlagNone)
{
#ifndef QT_DEBUG
    const_cast<QLoggingCategory &>(LOG_ALPINEAPK()).setEnabled(QtDebugMsg, false);
#endif

    // connections with our updater
    QObject::connect(m_updater, &AlpineApkUpdater::updatesCountChanged,
                     this, &AlpineApkBackend::updatesCountChanged);
    QObject::connect(m_updater, &AlpineApkUpdater::checkForUpdatesFinished,
                     this, &AlpineApkBackend::finishCheckForUpdates);
    QObject::connect(m_updater, &AlpineApkUpdater::fetchingUpdatesProgressChanged,
                     this, &AlpineApkBackend::setFetchingUpdatesProgress);

    // safety measure: make sure update check process can finish in some finite time
    QObject::connect(m_updatesTimeoutTimer, &QTimer::timeout,
                     this, &AlpineApkBackend::finishCheckForUpdates);
    m_updatesTimeoutTimer->setTimerType(Qt::CoarseTimer);
    m_updatesTimeoutTimer->setSingleShot(true);
    m_updatesTimeoutTimer->setInterval(5 * 60 * 1000); // 5 minutes

    // load packages data in a separate thread; it takes a noticeable amount of time
    //     and this way UI is not blocked here
    m_fetching = true; // we are busy!
    QFuture<void> loadResFuture = QtConcurrent::run([this]() {
        this->loadResources();
    });

    QObject::connect(&m_voidFutureWatcher, &QFutureWatcher<void>::finished,
                     this, &AlpineApkBackend::onLoadResourcesFinished);
    m_voidFutureWatcher.setFuture(loadResFuture);

    SourcesModel::global()->addSourcesBackend(new AlpineApkSourcesBackend(this));
}

// this fills in m_appStreamComponents
void AlpineApkBackend::loadAppStreamComponents()
{
    AppStream::Pool *appStreamPool = new AppStream::Pool();
#if ASQ_CHECK_VERSION(0, 15, 0)
    // use newer API and flags available only since 0.15.0
    appStreamPool->setFlags(AppStream::Pool::Flags(AppStream::Pool::Flag::FlagLoadOsCatalog | AppStream::Pool::Flag::FlagLoadOsDesktopFiles
                                                   | AppStream::Pool::Flag::FlagLoadOsMetainfo));

    // AS_FORMAT_STYLE_COLLECTION - Parse AppStream metadata collections (shipped by software distributors)
    appStreamPool->addExtraDataLocation(AppstreamDataDownloader::appStreamCacheDir(), AppStream::Metadata::FormatStyleCatalog);
#else
    appStreamPool->setFlags(AppStream::Pool::FlagReadCollection |
                            AppStream::Pool::FlagReadMetainfo |
                            AppStream::Pool::FlagReadDesktopFiles);
    appStreamPool->setCacheFlags(AppStream::Pool::CacheFlagUseUser |
                                 AppStream::Pool::CacheFlagUseSystem);
    appStreamPool->addMetadataLocation(AppstreamDataDownloader::appStreamCacheDir());
#endif

    if (!appStreamPool->load()) {
        qCWarning(LOG_ALPINEAPK) << "backend: Failed to load appstream data:"
                                 << appStreamPool->lastError();
    } else {
        m_appStreamComponents = appStreamPool->components();
        qCDebug(LOG_ALPINEAPK) << "backend: loaded AppStream metadata OK:"
                               << m_appStreamComponents.size() << "components.";
        // collect all categories present in appstream metadata
        // QSet<QString> collectedCategories;
        // for (const AppStream::Component &component : m_appStreamComponents) {
        //     const QStringList cats = component.categories();
        //     for (const QString &cat : cats) {
        //         collectedCategories.insert(cat);
        //     }
        // }
        // for (const QString &cat : collectedCategories) {
        //     qCDebug(LOG_ALPINEAPK) << "    collected category: " << cat;
        //     m_collectedCategories << cat;
        // }
    }
    delete appStreamPool;
}

// this uses m_appStreamComponents and m_availablePackages
//      to fill in m_resourcesAppstreamData
void AlpineApkBackend::parseAppStreamMetadata()
{
    if (m_availablePackages.size() > 0) {
        for (const QtApk::Package &pkg : std::as_const(m_availablePackages)) {
            // try to find appstream data for this package
            AppStream::Component appstreamComponent;
            for (const auto &appsC : std::as_const(m_appStreamComponents)) {
                // find result which package name is exactly the one we want
                if (appsC.packageNames().contains(pkg.name)) {
                    // workaround for kate (Kate Sessions is found first, but
                    //   package name = "kate" too, bugged metadata?)
                    if (pkg.name == QStringLiteral("kate")) {
                        // qCDebug(LOG_ALPINEAPK) << appsC.packageNames() << appsC.id();
                        // ^^ ("kate") "org.kde.plasma.katesessions"
                        if (appsC.id() != QStringLiteral("org.kde.kate")) {
                            continue;
                        }
                    }
                    appstreamComponent = appsC;
                    break; // exit for() loop
                }
            }

            const QString key = pkg.name.toLower();
            m_resourcesAppstreamData.insert(key, appstreamComponent);
        }
    }
}

static AbstractResource::Type toDiscoverResourceType(const AppStream::Component &component)
{
    AbstractResource::Type resType = AbstractResource::Type::Technical; // default
    // determine resource type here
    switch (component.kind()) {
    case AppStream::Component::KindDesktopApp:
    case AppStream::Component::KindConsoleApp:
    case AppStream::Component::KindWebApp:
        resType = AbstractResource::Type::Application;
        break;
    case AppStream::Component::KindAddon:
        resType = AbstractResource::Type::Addon;
        break;
    default:
        resType = AbstractResource::Type::Technical;
        break;
    }
    return resType;
}

void AlpineApkBackend::fillResourcesAndApplyAppStreamData()
{
    // now the tricky part - we need to reapply appstream component metadata to each resource
    if (m_availablePackages.size() > 0) {
        for (const QtApk::Package &pkg: m_availablePackages) {
            const QString key = pkg.name.toLower();

            AppStream::Component &appsComponent = m_resourcesAppstreamData[key];
            const AbstractResource::Type resType = toDiscoverResourceType(appsComponent);

            AlpineApkResource *res = m_resources.value(key, nullptr);
            if (res == nullptr) {
                // during first run of this function during initial load
                //   m_resources hash is empty, so we need to insert new items
                res = new AlpineApkResource(pkg, appsComponent, resType, this);
                res->setCategoryName(QStringLiteral("alpine_packages"));
                res->setOriginSource(QStringLiteral("apk"));
                res->setSection(QStringLiteral("dummy"));
                m_resources.insert(key, res);
                QObject::connect(res, &AlpineApkResource::stateChanged,
                                 this, &AlpineApkBackend::updatesCountChanged);
            } else {
                // this is not an initial run, just update existing resource
                res->setAppStreamData(appsComponent);
            }
        }
    }
}

void AlpineApkBackend::reloadAppStreamMetadata()
{
    // mark us as "Loading..."
    m_fetching = true;
    Q_EMIT fetchingChanged();

    loadAppStreamComponents();
    parseAppStreamMetadata();
    fillResourcesAndApplyAppStreamData();

    // mark us as "done loading"
    m_fetching = false;
    Q_EMIT fetchingChanged();
}

// this function is executed in the background thread
void AlpineApkBackend::loadResources()
{
    Q_EMIT this->passiveMessage(i18n("Loading, please wait..."));

    qCDebug(LOG_ALPINEAPK) << "backend: loading AppStream metadata...";

    loadAppStreamComponents();

    qCDebug(LOG_ALPINEAPK) << "backend: populating resources...";

    if (m_apkdb.open(QtApk::QTAPK_OPENF_READONLY)) {
        m_availablePackages = m_apkdb.getAvailablePackages();
        m_installedPackages = m_apkdb.getInstalledPackages();
        m_apkdb.close();
    }

    parseAppStreamMetadata();

    qCDebug(LOG_ALPINEAPK) << "  available" << m_availablePackages.size()
                           << "packages";
    qCDebug(LOG_ALPINEAPK) << "  installed" << m_installedPackages.size()
                           << "packages";
}

void AlpineApkBackend::onLoadResourcesFinished()
{
    qCDebug(LOG_ALPINEAPK) << "backend: appstream data loaded and sorted; fill in resources";

    fillResourcesAndApplyAppStreamData();

    // update "installed/not installed" state
    if (m_installedPackages.size() > 0) {
        for (const QtApk::Package &pkg: m_installedPackages) {
            const QString key = pkg.name.toLower();
            if (m_resources.contains(key)) {
                m_resources.value(key)->setState(AbstractResource::Installed);
            }
        }
    }

    qCDebug(LOG_ALPINEAPK) << "backend: resources loaded.";

    m_fetching = false;
    Q_EMIT fetchingChanged();
    // ^^ this causes the UI to update "Featured" page and show
    //    to user that we actually have loaded packages data

    // schedule check for updates 1 sec after we've loaded all resources
    QTimer::singleShot(1000, this, &AlpineApkBackend::checkForUpdates);

    // AppStream appdata downloader can download updated metadata files
    //    in a background thread. When potential download is finished,
    //    appstream data will be reloaded.
    m_appstreamDownloader = new AppstreamDataDownloader(nullptr);
    QObject::connect(m_appstreamDownloader, &AppstreamDataDownloader::downloadFinished,
                     this, &AlpineApkBackend::onAppstreamDataDownloaded, Qt::QueuedConnection);
    m_appstreamDownloader->start();
}

void AlpineApkBackend::onAppstreamDataDownloaded()
{
    if (m_appstreamDownloader) {
        if (m_appstreamDownloader->cacheWasUpdated()) {
            // it means we need to reload previously loaded appstream metadata
            // m_fetching is true if loadResources() is still executing
            //   in a background thread
            if (!m_fetching) {
                qCDebug(LOG_ALPINEAPK) << "AppStream metadata was updated; re-applying it to all resources";
                reloadAppStreamMetadata();
            } else {
                qCWarning(LOG_ALPINEAPK) << "AppStream metadata was updated, but cannot apply it: still fetching";
                // it should not really happen, but if it happens,
                //   then downloaded metadata will be used on the next
                //   discover launch anyway.
            }
        }
        delete m_appstreamDownloader;
        m_appstreamDownloader = nullptr;
    }
}

QVector<Category *> AlpineApkBackend::category() const
{
    static CategoryFilter s_apkFlt{CategoryFilter::FilterType::CategoryNameFilter, QLatin1String("alpine_packages")};

    // Display a single root category
    // we could add more, but Alpine apk does not have this concept
    static Category *s_rootCat = new Category(i18nc("Root category name", "Alpine Linux packages"), // name
                                              QStringLiteral("package-x-generic"), // icon name
                                              s_apkFlt, // const CategoryFilter& filters
                                              {displayName()}, // pluginName
                                              {}, // QVector<Category *> subCategories - none
                                              false // isAddons
    );

    return { s_rootCat };

    //    static QVector<Category *> s_cats;
    //    if (s_cats.isEmpty()) {
    //        // fill only once
    //        s_cats << s_rootCat;
    //        for (const QString &scat : m_collectedCategories) {
    //            Category *cat = new Category(
    //                        scat,   // name
    //                        QStringLiteral("package-x-generic"), // icon
    //                        {},     // orFilters
    //                        { displayName() }, // pluginName
    //                        {},     // subcategories
    //                        false   // isAddons
    //            );
    //            s_cats << cat;
    //        }
    //    }
    //    return s_cats;
    // ^^ causes deep hang in discover in recalculating QML bindings
}

int AlpineApkBackend::updatesCount() const
{
    return m_updater->updatesCount();
}

ResultsStream *AlpineApkBackend::search(const AbstractResourcesBackend::Filters &filter)
{
    QVector<StreamResult> ret;
    if (!filter.resourceUrl.isEmpty()) {
        return findResourceByPackageName(filter.resourceUrl);
    } else {
        for (AbstractResource *resource : std::as_const(m_resources)) {
            // skip technical package types (not apps/addons)
            //      that are not upgradeable
            //  (does not work because for now all Alpine packages are "technical"
            // if (resource->type() == AbstractResource::Technical
            //         && filter.state != AbstractResource::Upgradeable) {
            //     continue;
            // }

            // skip not-requested states
            if (resource->state() < filter.state) {
                continue;
            }

            if(resource->name().contains(filter.search, Qt::CaseInsensitive)
                    || resource->comment().contains(filter.search, Qt::CaseInsensitive)) {
                ret += resource;
            }
        }
    }
    return new ResultsStream(QStringLiteral("AlpineApkStream"), ret);
}

ResultsStream *AlpineApkBackend::findResourceByPackageName(const QUrl &searchUrl)
{
//    if (search.isLocalFile()) {
//        AlpineApkResource* res = new AlpineApkResource(
//                    search.fileName(), AbstractResource::Technical, this);
//        res->setSize(666);
//        res->setState(AbstractResource::None);
//        m_resources.insert(res->packageName(), res);
//        connect(res, &AlpineApkResource::stateChanged, this, &AlpineApkBackend::updatesCountChanged);
//        return new ResultsStream(QStringLiteral("AlpineApkStream-local"), { res });
//    }

    AlpineApkResource *result = nullptr;

    // QUrl("appstream://org.kde.krita.desktop")
    // smart workaround for appstream URLs - handle "featured" apps
    if (searchUrl.scheme()  == QLatin1String("appstream")) {
        // remove leading "org.kde."
        QString pkgName = searchUrl.host();
        if (pkgName.startsWith(QLatin1String("org.kde."))) {
            pkgName = pkgName.mid(8);
        }
        // remove trailing ".desktop"
        if (pkgName.endsWith(QLatin1String(".desktop"))) {
            pkgName = pkgName.left(pkgName.length() - 8);
        }
        // now we can search for "krita" package
        result = m_resources.value(pkgName);
    }

    // QUrl("apk://krita")
    // handle packages from Alpine repos
    if (searchUrl.scheme() == QLatin1String("apk")) {
        const QString pkgName = searchUrl.host();
        result = m_resources.value(pkgName);
    }

    if (!result) {
        return new ResultsStream(QStringLiteral("AlpineApkStream"), {});
    }
    return new ResultsStream(QStringLiteral("AlpineApkStream"), { result });
}

AbstractBackendUpdater *AlpineApkBackend::backendUpdater() const
{
    return m_updater;
}

AbstractReviewsBackend *AlpineApkBackend::reviewsBackend() const
{
    return m_reviews;
}

Transaction* AlpineApkBackend::installApplication(AbstractResource *app, const AddonList &addons)
{
    return new AlpineApkTransaction(qobject_cast<AlpineApkResource *>(app),
                                    addons, Transaction::InstallRole);
}

Transaction* AlpineApkBackend::installApplication(AbstractResource *app)
{
    return new AlpineApkTransaction(qobject_cast<AlpineApkResource *>(app),
                                    Transaction::InstallRole);
}

Transaction* AlpineApkBackend::removeApplication(AbstractResource *app)
{
    return new AlpineApkTransaction(qobject_cast<AlpineApkResource *>(app),
                                    Transaction::RemoveRole);
}

int AlpineApkBackend::fetchingUpdatesProgress() const
{
    if (!m_fetching) return 100;
    return m_fetchProgress;
}

void AlpineApkBackend::checkForUpdates()
{
    if (m_fetching) {
        qCDebug(LOG_ALPINEAPK) << "backend: checkForUpdates(): already fetching";
        return;
    }

    qCDebug(LOG_ALPINEAPK) << "backend: start checkForUpdates()";

    // safety measure - finish updates check in some time
    m_updatesTimeoutTimer->start();

    // let our updater do the job
    m_updater->startCheckForUpdates();

    // update UI
    m_fetching = true;
    m_fetchProgress = 0;
    Q_EMIT fetchingChanged();
    Q_EMIT fetchingUpdatesProgressChanged();
}

void AlpineApkBackend::finishCheckForUpdates()
{
    m_updatesTimeoutTimer->stop(); // stop safety timer
    // update UI
    m_fetching = false;
    Q_EMIT fetchingChanged();
    Q_EMIT fetchingUpdatesProgressChanged();
}

QString AlpineApkBackend::displayName() const
{
    return i18nc("Backend plugin display name", "Alpine APK");
}

bool AlpineApkBackend::hasApplications() const
{
    return true;
}

void AlpineApkBackend::setFetchingUpdatesProgress(int percent)
{
    m_fetchProgress = percent;
    Q_EMIT fetchingUpdatesProgressChanged();
}

// needed because DISCOVER_BACKEND_PLUGIN(AlpineApkBackend) contains Q_OBJECT
#include "AlpineApkBackend.moc"
