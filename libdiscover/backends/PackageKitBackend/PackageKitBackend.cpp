/*
 *   SPDX-FileCopyrightText: 2012 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *   SPDX-FileCopyrightText: 2013 Lukas Appelhans <l.appelhans@gmx.de>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "PackageKitBackend.h"
#include "AppPackageKitResource.h"
#include "LocalFilePKResource.h"
#include "PKResolveTransaction.h"
#include "PKTransaction.h"
#include "PackageKitSourcesBackend.h"
#include "PackageKitUpdater.h"
#include <AppStreamQt/release.h>
#include <AppStreamQt/utils.h>
#include <AppStreamQt/version.h>
#include <appstream/AppStreamIntegration.h>
#include <appstream/AppStreamUtils.h>
#include <appstream/OdrsReviewsBackend.h>
#include <resources/AbstractResource.h>
#include <resources/SourcesModel.h>
#include <resources/StandardBackendUpdater.h>

#include <QDebug>
#include <QFileSystemWatcher>
#include <QFutureWatcher>
#include <QMimeDatabase>
#include <QProcess>
#include <QStandardPaths>
#include <QStringList>
#include <QtConcurrentRun>

#include <PackageKit/Daemon>
#include <PackageKit/Details>
#include <PackageKit/Offline>

#include <KLocalizedString>
#include <KProtocolManager>

#include "config-paths.h"
#include "libdiscover_backend_debug.h"
#include "utils.h"
#include <Category/Category.h>

DISCOVER_BACKEND_PLUGIN(PackageKitBackend)

template<typename T, typename W>
static void setWhenAvailable(const QDBusPendingReply<T> &pending, W func, QObject *parent)
{
    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(pending, parent);
    QObject::connect(watcher, &QDBusPendingCallWatcher::finished, parent, [func](QDBusPendingCallWatcher *watcher) {
        watcher->deleteLater();
        QDBusPendingReply<T> reply = *watcher;
        func(reply.value());
    });
}

QString PackageKitBackend::locateService(const QString &filename)
{
    return QStandardPaths::locate(QStandardPaths::GenericDataLocation, QStringLiteral("applications/") + filename);
}

PackageKitBackend::PackageKitBackend(QObject *parent)
    : AbstractResourcesBackend(parent)
    , m_appdata(new AppStream::Pool)
    , m_updater(new PackageKitUpdater(this))
    , m_refresher(nullptr)
    , m_isFetching(0)
    , m_reviews(AppStreamIntegration::global()->reviews())
{
    QTimer *t = new QTimer(this);
    connect(t, &QTimer::timeout, this, &PackageKitBackend::checkForUpdates);
    t->setInterval(60 * 60 * 1000);
    t->setSingleShot(false);
    t->start();

    m_delayedDetailsFetch.setSingleShot(true);
    m_delayedDetailsFetch.setInterval(100);
    connect(&m_delayedDetailsFetch, &QTimer::timeout, this, &PackageKitBackend::performDetailsFetch);

    connect(PackageKit::Daemon::global(), &PackageKit::Daemon::restartScheduled, m_updater, &PackageKitUpdater::enableNeedsReboot);
    connect(PackageKit::Daemon::global(), &PackageKit::Daemon::isRunningChanged, this, &PackageKitBackend::checkDaemonRunning);
    connect(m_reviews.data(), &OdrsReviewsBackend::ratingsReady, this, [this] {
        m_reviews->emitRatingFetched(this, kTransform<QList<AbstractResource *>>(m_packages.packages, [](AbstractResource *r) {
                                         return r;
                                     }));
    });

    auto proxyWatch = new QFileSystemWatcher(this);
    proxyWatch->addPath(QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) + QLatin1String("/kioslaverc"));
    connect(proxyWatch, &QFileSystemWatcher::fileChanged, this, [this]() {
        KProtocolManager::reparseConfiguration();
        updateProxy();
    });

    SourcesModel::global()->addSourcesBackend(new PackageKitSourcesBackend(this));

    reloadPackageList();

    acquireFetching(true);
    setWhenAvailable(
        PackageKit::Daemon::getTimeSinceAction(PackageKit::Transaction::RoleRefreshCache),
        [this](uint timeSince) {
            if (timeSince > 3600)
                checkForUpdates();
            else
                fetchUpdates();
            acquireFetching(false);
        },
        this);

    PackageKit::Daemon::global()->setHints(QStringList() << QStringLiteral("interactive=true")
                                                         << QStringLiteral("locale=%1").arg(qEnvironmentVariable("LANG")));
}

PackageKitBackend::~PackageKitBackend()
{
    m_threadPool.waitForDone(200);
    m_threadPool.clear();
}

void PackageKitBackend::updateProxy()
{
    if (PackageKit::Daemon::isRunning()) {
        static bool everHad = KProtocolManager::useProxy();
        if (!everHad && !KProtocolManager::useProxy())
            return;

        everHad = KProtocolManager::useProxy();
        PackageKit::Daemon::global()->setProxy(KProtocolManager::proxyFor(QStringLiteral("http")),
                                               KProtocolManager::proxyFor(QStringLiteral("https")),
                                               KProtocolManager::proxyFor(QStringLiteral("ftp")),
                                               KProtocolManager::proxyFor(QStringLiteral("socks")),
                                               {},
                                               {});
    }
}

bool PackageKitBackend::isFetching() const
{
    return m_isFetching;
}

void PackageKitBackend::acquireFetching(bool f)
{
    if (f)
        m_isFetching++;
    else
        m_isFetching--;

    if ((!f && m_isFetching == 0) || (f && m_isFetching == 1)) {
        Q_EMIT fetchingChanged();
        if (m_isFetching == 0)
            Q_EMIT available();
    }
    Q_ASSERT(m_isFetching >= 0);
}

struct DelayedAppStreamLoad {
    QVector<AppStream::Component> components;
    QHash<QString, AppStream::Component> missingComponents;
    bool correct = true;
};

static DelayedAppStreamLoad loadAppStream(AppStream::Pool *appdata)
{
    DelayedAppStreamLoad ret;

    ret.correct = appdata->load();
    if (!ret.correct) {
        qWarning() << "Could not open the AppStream metadata pool" << appdata->lastError();
    }

    const auto components = appdata->components();
    ret.components.reserve(components.size());
    for (const AppStream::Component &component : components) {
        if (component.kind() == AppStream::Component::KindFirmware)
            continue;

        const auto pkgNames = component.packageNames();
        if (pkgNames.isEmpty()) {
            const auto entries = component.launchable(AppStream::Launchable::KindDesktopId).entries();
            if (component.kind() == AppStream::Component::KindDesktopApp && !entries.isEmpty()) {
                const QString file = PackageKitBackend::locateService(entries.first());
                if (!file.isEmpty()) {
                    ret.missingComponents[file] = component;
                }
            }
        } else {
            ret.components << component;
        }
    }
    return ret;
}

void PackageKitBackend::reloadPackageList()
{
    acquireFetching(true);
    if (m_refresher) {
        disconnect(m_refresher.data(), &PackageKit::Transaction::finished, this, &PackageKitBackend::reloadPackageList);
    }

    m_appdata.reset(new AppStream::Pool);

    auto fw = new QFutureWatcher<DelayedAppStreamLoad>(this);
    connect(fw, &QFutureWatcher<DelayedAppStreamLoad>::finished, this, [this, fw]() {
        const auto data = fw->result();
        fw->deleteLater();

        if (!data.correct && m_packages.packages.isEmpty()) {
            QTimer::singleShot(0, this, [this]() {
                Q_EMIT passiveMessage(i18n("Please make sure that Appstream is properly set up on your system"));
            });
        }
        for (const auto &component : data.components) {
            addComponent(component);
        }

        if (data.components.isEmpty()) {
            qCDebug(LIBDISCOVER_BACKEND_LOG) << "empty appstream db";
            if (PackageKit::Daemon::backendName() == QLatin1String("aptcc") || PackageKit::Daemon::backendName().isEmpty()) {
                checkForUpdates();
            }
        }
        if (!m_appstreamInitialized) {
            m_appstreamInitialized = true;
            Q_EMIT loadedAppStream();
        }
        acquireFetching(false);

        const QList<AppStream::Component> distroComponents = m_appdata->componentsById(AppStream::Utils::currentDistroComponentId());
        if (distroComponents.isEmpty()) {
            qWarning() << "no component found for" << AppStream::Utils::currentDistroComponentId();
        }
        for (const AppStream::Component &dc : distroComponents) {
            const auto releases = dc.releases();
            for (const auto &r : releases) {
                int cmp = AppStream::Utils::vercmpSimple(r.version(), AppStreamIntegration::global()->osRelease()->versionId());
                if (cmp == 0) {
                    // Ignore (likely) empty date_eol entries that are parsed as the UNIX Epoch
                    if (r.timestampEol().isNull() || r.timestampEol().toSecsSinceEpoch() == 0) {
                        continue;
                    }
                    if (r.timestampEol() < QDateTime::currentDateTime()) {
                        const QString releaseDate = QLocale().toString(r.timestampEol());
                        Q_EMIT inlineMessageChanged(
                            QSharedPointer<InlineMessage>::create(InlineMessage::Warning,
                                                                  QStringLiteral("dialog-warning"),
                                                                  i18nc("%1 is the date as formatted by the locale",
                                                                        "Your operating system ended support on %1. Consider upgrading to a supported version.",
                                                                        releaseDate)));
                    }
                }
            }
        }
    });
    fw->setFuture(QtConcurrent::run(&m_threadPool, &loadAppStream, m_appdata.get()));
}

AppPackageKitResource *PackageKitBackend::addComponent(const AppStream::Component &component)
{
    Q_ASSERT(isFetching());
    const QStringList pkgNames = component.packageNames();
    Q_ASSERT(!pkgNames.isEmpty());

    auto &resPos = m_packages.packages[component.id()];
    AppPackageKitResource *res = qobject_cast<AppPackageKitResource *>(resPos);
    if (!res) {
        res = new AppPackageKitResource(component, pkgNames.at(0), this);
        resPos = res;
    } else {
        res->clearPackageIds();
    }
    for (const QString &pkg : pkgNames) {
        m_packages.packageToApp[pkg] += component.id();
    }

    const auto componentExtends = component.extends();
    for (const QString &pkg : componentExtends) {
        m_packages.extendedBy[pkg] += res;
    }
    return res;
}

void PackageKitBackend::resolvePackages(const QStringList &packageNames)
{
    if (!m_resolveTransaction) {
        m_resolveTransaction = new PKResolveTransaction(this);
        connect(m_resolveTransaction, &PKResolveTransaction::allFinished, this, &PackageKitBackend::getPackagesFinished);
        connect(m_resolveTransaction, &PKResolveTransaction::started, this, [this] {
            m_resolveTransaction = nullptr;
        });
    }

    m_resolveTransaction->addPackageNames(packageNames);
}

void PackageKitBackend::fetchUpdates()
{
    if (m_updater->isProgressing())
        return;

    m_getUpdatesTransaction = PackageKit::Daemon::getUpdates();
    connect(m_getUpdatesTransaction, &PackageKit::Transaction::finished, this, &PackageKitBackend::getUpdatesFinished);
    connect(m_getUpdatesTransaction, &PackageKit::Transaction::package, this, &PackageKitBackend::addPackageToUpdate);
    connect(m_getUpdatesTransaction, &PackageKit::Transaction::errorCode, this, &PackageKitBackend::transactionError);
    connect(m_getUpdatesTransaction, &PackageKit::Transaction::percentageChanged, this, &PackageKitBackend::fetchingUpdatesProgressChanged);
    m_updatesPackageId.clear();
    m_hasSecurityUpdates = false;

    m_updater->setProgressing(true);

    Q_EMIT fetchingUpdatesProgressChanged();
}

void PackageKitBackend::addPackageArch(PackageKit::Transaction::Info info, const QString &packageId, const QString &summary)
{
    addPackage(info, packageId, summary, true);
}

void PackageKitBackend::addPackageNotArch(PackageKit::Transaction::Info info, const QString &packageId, const QString &summary)
{
    addPackage(info, packageId, summary, false);
}

void PackageKitBackend::addPackage(PackageKit::Transaction::Info info, const QString &packageId, const QString &summary, bool arch)
{
    if (PackageKit::Daemon::packageArch(packageId) == QLatin1String("source")) {
        // We do not add source packages, they make little sense here. If source is needed,
        // we are going to have to consider that in some other way, some other time
        // If we do not ignore them here, e.g. openSuse entirely fails at installing applications
        return;
    }
    const QString packageName = PackageKit::Daemon::packageName(packageId);
    QSet<AbstractResource *> r = resourcesByPackageName(packageName);
    if (r.isEmpty()) {
        auto pk = new PackageKitResource(packageName, summary, this);
        r = {pk};
        m_packagesToAdd.insert(pk);
    }
    for (auto res : qAsConst(r))
        static_cast<PackageKitResource *>(res)->addPackageId(info, packageId, arch);
}

void PackageKitBackend::getPackagesFinished()
{
    includePackagesToAdd();
}

void PackageKitBackend::includePackagesToAdd()
{
    if (m_packagesToAdd.isEmpty() && m_packagesToDelete.isEmpty())
        return;

    acquireFetching(true);
    for (PackageKitResource *res : qAsConst(m_packagesToAdd)) {
        m_packages.packages[res->packageName()] = res;
    }
    for (PackageKitResource *res : qAsConst(m_packagesToDelete)) {
        const auto pkgs = m_packages.packageToApp.value(res->packageName(), {res->packageName()});
        for (const auto &pkg : pkgs) {
            auto res = m_packages.packages.take(pkg);
            if (res) {
                if (AppPackageKitResource *ares = qobject_cast<AppPackageKitResource *>(res)) {
                    const auto extends = res->extends();
                    for (const auto &ext : extends)
                        m_packages.extendedBy[ext].removeAll(ares);
                }

                Q_EMIT resourceRemoved(res);
                res->deleteLater();
            }
        }
    }
    m_packagesToAdd.clear();
    m_packagesToDelete.clear();
    acquireFetching(false);
}

void PackageKitBackend::transactionError(PackageKit::Transaction::Error, const QString &message)
{
    qWarning() << "Transaction error: " << message << sender();
    Q_EMIT passiveMessage(message);
}

void PackageKitBackend::packageDetails(const PackageKit::Details &details)
{
    const QSet<AbstractResource *> resources = resourcesByPackageName(PackageKit::Daemon::packageName(details.packageId()));
    if (resources.isEmpty())
        qWarning() << "couldn't find package for" << details.packageId();

    for (AbstractResource *res : resources) {
        qobject_cast<PackageKitResource *>(res)->setDetails(details);
    }
}

QSet<AbstractResource *> PackageKitBackend::resourcesByPackageName(const QString &name) const
{
    return resourcesByPackageNames<QSet<AbstractResource *>>(QStringList{name});
}

template<typename T, typename W>
T PackageKitBackend::resourcesByPackageNames(const W &pkgnames) const
{
    T ret;
    ret.reserve(pkgnames.size());
    for (const QString &name : pkgnames) {
        const QStringList names = m_packages.packageToApp.value(name, QStringList(name));
        for (const QString &name : names) {
            AbstractResource *res = m_packages.packages.value(name);
            if (res)
                ret += res;
        }
    }
    return ret;
}

void PackageKitBackend::checkForUpdates()
{
    if (PackageKit::Daemon::global()->offline()->updateTriggered()) {
        qCDebug(LIBDISCOVER_BACKEND_LOG) << "Won't be checking for updates again, the system needs a reboot to apply the fetched offline updates.";
        return;
    }

    if (!m_refresher) {
        acquireFetching(true);
        m_refresher = PackageKit::Daemon::refreshCache(false);

        connect(m_refresher.data(), &PackageKit::Transaction::errorCode, this, &PackageKitBackend::transactionError);
        connect(m_refresher.data(), &PackageKit::Transaction::finished, this, [this]() {
            m_refresher = nullptr;
            fetchUpdates();
            acquireFetching(false);
        });
    } else {
        qWarning() << "already resetting";
    }
}

QList<AppStream::Component> PackageKitBackend::componentsById(const QString &id) const
{
    Q_ASSERT(m_appstreamInitialized);
    auto comps = m_appdata->componentsById(id);
    if (comps.isEmpty()) {
        comps = m_appdata->componentsByProvided(AppStream::Provided::KindId, id);
    }
    return comps;
}

static const auto needsResolveFilter = [](AbstractResource *res) {
    return res->state() == AbstractResource::Broken;
};

class PKResultsStream : public ResultsStream
{
public:
    PKResultsStream(PackageKitBackend *backend, const QString &name)
        : ResultsStream(name)
        , backend(backend)
    {
    }

    PKResultsStream(PackageKitBackend *backend, const QString &name, const QVector<AbstractResource *> &resources)
        : ResultsStream(name)
        , backend(backend)
    {
        QTimer::singleShot(0, this, [resources, this]() {
            setResources(resources);
            finish();
        });
    }

    void setResources(const QVector<AbstractResource *> &res)
    {
        if (res.isEmpty())
            return;

        Q_ASSERT(res.size() == QSet(res.constBegin(), res.constEnd()).size());
        const auto toResolve = kFilter<QVector<AbstractResource *>>(res, needsResolveFilter);
        if (!toResolve.isEmpty())
            backend->resolvePackages(kTransform<QStringList>(toResolve, [](AbstractResource *res) {
                return res->packageName();
            }));
        Q_EMIT resourcesFound(res);
    }

private:
    PackageKitBackend *const backend;
};

ResultsStream *PackageKitBackend::search(const AbstractResourcesBackend::Filters &filter)
{
    if (!filter.resourceUrl.isEmpty()) {
        return findResourceByPackageName(filter.resourceUrl);
    } else if (!filter.extends.isEmpty()) {
        auto stream = new PKResultsStream(this, QStringLiteral("PackageKitStream-extends"));
        auto f = [this, filter, stream] {
            const auto resources = kTransform<QVector<AbstractResource *>>(m_packages.extendedBy.value(filter.extends), [](AppPackageKitResource *a) {
                return a;
            });
            stream->setResources(resources);
            stream->finish();
        };
        runWhenInitialized(f, stream);
        return stream;
    } else if (filter.state == AbstractResource::Upgradeable) {
        return new ResultsStream(QStringLiteral("PackageKitStream-upgradeable"),
                                 kTransform<QVector<AbstractResource *>>(upgradeablePackages())); // No need for it to be a PKResultsStream
    } else if (filter.state == AbstractResource::Installed) {
        auto stream = new PKResultsStream(this, QStringLiteral("PackageKitStream-installed"));
        auto f = [this, stream, filter] {
            const auto toResolve = kFilter<QVector<AbstractResource *>>(m_packages.packages, needsResolveFilter);

            auto installedAndNameFilter = [filter](AbstractResource *res) {
                return res->state() >= AbstractResource::Installed && !qobject_cast<PackageKitResource *>(res)->isCritical()
                    && (res->name().contains(filter.search, Qt::CaseInsensitive) || res->packageName().compare(filter.search, Qt::CaseInsensitive) == 0);
            };
            bool furtherSearch = false;
            if (!toResolve.isEmpty()) {
                resolvePackages(kTransform<QStringList>(toResolve, [](AbstractResource *res) {
                    return res->packageName();
                }));
                connect(m_resolveTransaction, &PKResolveTransaction::allFinished, this, [stream, toResolve, installedAndNameFilter] {
                    const auto resolved = kFilter<QVector<AbstractResource *>>(toResolve, installedAndNameFilter);
                    if (!resolved.isEmpty())
                        Q_EMIT stream->resourcesFound(resolved);
                    stream->finish();
                });
                furtherSearch = true;
            }

            const auto resolved = kFilter<QVector<AbstractResource *>>(m_packages.packages, installedAndNameFilter);
            if (!resolved.isEmpty()) {
                QTimer::singleShot(0, this, [resolved, toResolve, stream]() {
                    if (!resolved.isEmpty())
                        Q_EMIT stream->resourcesFound(resolved);

                    if (toResolve.isEmpty())
                        stream->finish();
                });
                furtherSearch = true;
            }

            if (!furtherSearch)
                stream->finish();
        };
        runWhenInitialized(f, stream);
        return stream;
    } else if (filter.search.isEmpty() && !filter.category) {
        auto stream = new PKResultsStream(this, QStringLiteral("PackageKitStream-all"));
        auto f = [this, filter, stream] {
            auto resources = kFilter<QVector<AbstractResource *>>(m_packages.packages, [](AbstractResource *res) {
                return res->type() != AbstractResource::Technical && !qobject_cast<PackageKitResource *>(res)->isCritical()
                    && !qobject_cast<PackageKitResource *>(res)->extendsItself();
            });
            stream->setResources(resources);
            stream->finish();
        };
        runWhenInitialized(f, stream);
        return stream;
    } else {
        auto stream = new PKResultsStream(this, QStringLiteral("PackageKitStream-search"));
        const auto f = [this, stream, filter]() {
            const auto components = !filter.search.isEmpty() ? m_appdata->search(filter.search)
#if ASQ_MAJOR_VERSION >= 0 && ASQ_MINOR_VERSION >= 15 && ASQ_MICRO_VERSION >= 6
                                  : filter.category          ? AppStreamUtils::componentsByCategories(m_appdata.get(),
                                                                                                      filter.category,
                                                                                                      AppStream::Bundle::KindUnknown)
#endif
                                                             : m_appdata->components();
            const QSet<QString> ids = kTransform<QSet<QString>>(components, [](const AppStream::Component &comp) {
                return comp.id();
            });
            if (!ids.isEmpty()) {
                const auto resources =
                    kFilter<QVector<AbstractResource *>>(resourcesByPackageNames<QVector<AbstractResource *>>(ids), [](AbstractResource *res) {
                        return !qobject_cast<PackageKitResource *>(res)->extendsItself();
                    });
                stream->setResources(resources);
            }
            stream->finish();
        };
        runWhenInitialized(f, stream);
        return stream;
    }
}

void PackageKitBackend::runWhenInitialized(const std::function<void()> &f, QObject *stream)
{
    if (!m_appstreamInitialized) {
        connect(this, &PackageKitBackend::loadedAppStream, stream, f);
    } else {
        QTimer::singleShot(0, this, f);
    }
}

PKResultsStream *PackageKitBackend::findResourceByPackageName(const QUrl &url)
{
    if (url.isLocalFile()) {
        QMimeDatabase db;
        const auto mime = db.mimeTypeForUrl(url);
        if (mime.inherits(QStringLiteral("application/vnd.debian.binary-package")) //
            || mime.inherits(QStringLiteral("application/x-rpm")) //
            || mime.inherits(QStringLiteral("application/x-tar")) //
            || mime.inherits(QStringLiteral("application/x-zstd-compressed-tar")) //
            || mime.inherits(QStringLiteral("application/x-xz-compressed-tar"))) {
            return new PKResultsStream(this, QStringLiteral("PackageKitStream-localpkg"), {new LocalFilePKResource(url, this)});
        }
    } else if (url.scheme() == QLatin1String("appstream")) {
        const auto appstreamIds = AppStreamUtils::appstreamIds(url);
        if (appstreamIds.isEmpty())
            Q_EMIT passiveMessage(i18n("Malformed appstream url '%1'", url.toDisplayString()));
        else {
            auto stream = new PKResultsStream(this, QStringLiteral("PackageKitStream-appstream-url"));
            const auto f = [this, appstreamIds, stream]() {
                for (const auto &appstreamId : appstreamIds) {
                    const auto comps = componentsById(appstreamId);
                    if (comps.isEmpty()) {
                        continue;
                    }
                    stream->setResources(resourcesByComponents<QVector<AbstractResource *>>(comps));
                }
                stream->finish();
            };
            runWhenInitialized(f, stream);
            return stream;
        }
    }
    return new PKResultsStream(this, QStringLiteral("PackageKitStream-unknown-url"), {});
}

template<typename T>
T PackageKitBackend::resourcesByComponents(const QList<AppStream::Component> &comps) const
{
    T ret;
    ret.reserve(comps.size());
    QSet<QString> done;
    for (const auto &comp : comps) {
        if (comp.packageNames().isEmpty() || done.contains(comp.id())) {
            continue;
        }
        done += comp.id();
        ret << m_packages.packages.value(comp.id());
        Q_ASSERT(ret.constLast());
    }
    return ret;
}

bool PackageKitBackend::hasSecurityUpdates() const
{
    return m_hasSecurityUpdates;
}

int PackageKitBackend::updatesCount() const
{
    if (PackageKit::Daemon::global()->offline()->updateTriggered())
        return 0;

    int ret = 0;
    QSet<QString> packages;
    const auto toUpgrade = upgradeablePackages();
    for (auto res : toUpgrade) {
        const auto packageName = res->packageName();
        if (packages.contains(packageName)) {
            continue;
        }
        packages.insert(packageName);
        ret += 1;
    }
    return ret;
}

Transaction *PackageKitBackend::installApplication(AbstractResource *app, const AddonList &addons)
{
    Transaction *t = nullptr;
    if (!addons.addonsToInstall().isEmpty()) {
        QVector<AbstractResource *> appsToInstall = resourcesByPackageNames<QVector<AbstractResource *>>(addons.addonsToInstall());
        if (!app->isInstalled())
            appsToInstall << app;
        t = new PKTransaction(appsToInstall, Transaction::ChangeAddonsRole);
    } else if (!app->isInstalled())
        t = installApplication(app);

    if (!addons.addonsToRemove().isEmpty()) {
        const auto appsToRemove = resourcesByPackageNames<QVector<AbstractResource *>>(addons.addonsToRemove());
        t = new PKTransaction(appsToRemove, Transaction::RemoveRole);
    }

    return t;
}

Transaction *PackageKitBackend::installApplication(AbstractResource *app)
{
    return new PKTransaction({app}, Transaction::InstallRole);
}

Transaction *PackageKitBackend::removeApplication(AbstractResource *app)
{
    Q_ASSERT(!isFetching());
    if (!qobject_cast<PackageKitResource *>(app)) {
        Q_EMIT passiveMessage(i18n("Cannot remove '%1'", app->name()));
        return nullptr;
    }
    return new PKTransaction({app}, Transaction::RemoveRole);
}

QSet<AbstractResource *> PackageKitBackend::upgradeablePackages() const
{
    if (isFetching() || !m_packagesToAdd.isEmpty()) {
        return {};
    }

    QSet<AbstractResource *> ret;
    ret.reserve(m_updatesPackageId.size());
    for (const QString &pkgid : qAsConst(m_updatesPackageId)) {
        const QString pkgname = PackageKit::Daemon::packageName(pkgid);
        const auto pkgs = resourcesByPackageName(pkgname);
        if (pkgs.isEmpty()) {
            qWarning() << "couldn't find resource for" << pkgid;
        }
        ret.unite(pkgs);
    }
    return kFilter<QSet<AbstractResource *>>(ret, [](AbstractResource *res) {
        return !static_cast<PackageKitResource *>(res)->extendsItself();
    });
}

void PackageKitBackend::addPackageToUpdate(PackageKit::Transaction::Info info, const QString &packageId, const QString &summary)
{
    if (info == PackageKit::Transaction::InfoBlocked) {
        return;
    }

    if (info == PackageKit::Transaction::InfoRemoving || info == PackageKit::Transaction::InfoObsoleting) {
        // Don't try updating packages which need to be removed
        return;
    }

    if (info == PackageKit::Transaction::InfoSecurity)
        m_hasSecurityUpdates = true;

    m_updatesPackageId += packageId;
    addPackage(info, packageId, summary, true);
}

void PackageKitBackend::getUpdatesFinished(PackageKit::Transaction::Exit, uint)
{
    if (!m_updatesPackageId.isEmpty()) {
        resolvePackages(kTransform<QStringList>(m_updatesPackageId, [](const QString &pkgid) {
            return PackageKit::Daemon::packageName(pkgid);
        }));
    }

    m_updater->setProgressing(false);

    includePackagesToAdd();
    if (isFetching()) {
        auto a = new OneTimeAction(
            [this] {
                Q_EMIT updatesCountChanged();
            },
            this);
        connect(this, &PackageKitBackend::available, a, &OneTimeAction::trigger);
    } else
        Q_EMIT updatesCountChanged();
}

bool PackageKitBackend::isPackageNameUpgradeable(const PackageKitResource *res) const
{
    return !upgradeablePackageId(res).isEmpty();
}

// Copy of Transaction::packageName that doesn't create a copy but just pass a reference
// It's an optimisation as there's a bunch of allocations that happen from packageName
// Having packageName return a QStringRef or a QStringView would fix this issue.
// TODO Qt 6: Have packageName and similars return a QStringView
static QStringView TransactionpackageName(const QString &packageID)
{
    return QStringView(packageID).left(packageID.indexOf(QLatin1Char(';')));
}

QSet<QString> PackageKitBackend::upgradeablePackageId(const PackageKitResource *res) const
{
    QSet<QString> ids;
    const QString name = res->packageName();
    for (const QString &pkgid : m_updatesPackageId) {
        if (TransactionpackageName(pkgid) == name)
            ids.insert(pkgid);
    }
    return ids;
}

void PackageKitBackend::fetchDetails(const QSet<QString> &pkgid)
{
    if (!m_delayedDetailsFetch.isActive()) {
        m_delayedDetailsFetch.start();
    }

    m_packageNamesToFetchDetails += pkgid;
}

void PackageKitBackend::performDetailsFetch()
{
    Q_ASSERT(!m_packageNamesToFetchDetails.isEmpty());
    const auto ids = m_packageNamesToFetchDetails.values();

    PackageKit::Transaction *transaction = PackageKit::Daemon::getDetails(ids);
    connect(transaction, &PackageKit::Transaction::details, this, &PackageKitBackend::packageDetails);
    connect(transaction, &PackageKit::Transaction::errorCode, this, &PackageKitBackend::transactionError);
    m_packageNamesToFetchDetails.clear();
}

void PackageKitBackend::checkDaemonRunning()
{
    if (!PackageKit::Daemon::isRunning()) {
        qWarning() << "PackageKit stopped running!";
    } else
        updateProxy();
}

AbstractBackendUpdater *PackageKitBackend::backendUpdater() const
{
    return m_updater;
}

QVector<AppPackageKitResource *> PackageKitBackend::extendedBy(const QString &id) const
{
    return m_packages.extendedBy[id];
}

AbstractReviewsBackend *PackageKitBackend::reviewsBackend() const
{
    return m_reviews.data();
}

QString PackageKitBackend::displayName() const
{
    return AppStreamIntegration::global()->osRelease()->prettyName();
}

int PackageKitBackend::fetchingUpdatesProgress() const
{
    if (!m_getUpdatesTransaction)
        return 0;

    if (m_getUpdatesTransaction->status() == PackageKit::Transaction::StatusWait
        || m_getUpdatesTransaction->status() == PackageKit::Transaction::StatusUnknown) {
        return m_getUpdatesTransaction->property("lastPercentage").toInt();
    }
    int percentage = percentageWithStatus(m_getUpdatesTransaction->status(), m_getUpdatesTransaction->percentage());
    m_getUpdatesTransaction->setProperty("lastPercentage", percentage);
    return percentage;
}

HelpfulError *PackageKitBackend::explainDysfunction() const
{
    const auto error = m_appdata->lastError();
    if (!error.isEmpty()) {
        return new HelpfulError(QStringLiteral("network-disconnect"), error);
    }
    return AbstractResourcesBackend::explainDysfunction();
}

#include "PackageKitBackend.moc"
