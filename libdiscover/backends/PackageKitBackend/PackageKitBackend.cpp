/*
 *   SPDX-FileCopyrightText: 2012 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *   SPDX-FileCopyrightText: 2013 Lukas Appelhans <l.appelhans@gmx.de>
 *   SPDX-FileCopyrightText: 2023 Harald Sitter <sitter@kde.org>
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
#include <AppStreamQt/systeminfo.h>
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
#include <QHash>
#include <QMimeDatabase>
#include <QProcess>
#include <QStandardPaths>
#include <QStringList>
#include <QtConcurrentRun>

#include <PackageKit/Daemon>
#include <PackageKit/Details>
#include <PackageKit/Offline>

#include <KConfig>
#include <KConfigGroup>
#include <KLocalizedString>
#include <KProtocolManager>

#include <optional>

#include "config-paths.h"
#include "libdiscover_backend_debug.h"
#include "utils.h"
#include <Category/Category.h>
#include <resources/ResourcesModel.h>

using namespace Qt::StringLiterals;

DISCOVER_BACKEND_PLUGIN(PackageKitBackend)

QDebug operator<<(QDebug dbg, const PackageOrAppId &value)
{
    QDebugStateSaver saver(dbg);
    dbg.nospace();
    dbg << "PackageOrAppId(";
    dbg << "id: " << value.id << ',';
    dbg << "isPkg: " << value.isPackageName;
    dbg << ')';
    return dbg;
}

bool operator==(const PackageOrAppId &a, const PackageOrAppId &b)
{
    return a.isPackageName == b.isPackageName && a.id == b.id;
}

uint qHash(const PackageOrAppId &id, uint seed)
{
    return qHash(id.id, seed) ^ qHash(id.isPackageName, seed);
}

PackageOrAppId makeAppId(const QString &id)
{
    return {id, false};
}

PackageOrAppId makePackageId(const QString &id)
{
    return {id, true};
}

PackageOrAppId makeResourceId(PackageKitResource *res)
{
    auto appstreamResource = qobject_cast<AppPackageKitResource *>(res);
    if (appstreamResource) {
        return {appstreamResource->appstreamId(), false};
    }
    return {res->packageName(), true};
}

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

Delay::Delay()
{
    m_delay.setSingleShot(true);
    m_delay.setInterval(100);

    connect(&m_delay, &QTimer::timeout, this, [this] {
        Q_EMIT perform(m_pkgids);
        m_pkgids.clear();
    });
}

PackageKitBackend::PackageKitBackend(QObject *parent)
    : AbstractResourcesBackend(parent)
    , m_appdata(new AppStream::Pool)
    , m_updater(new PackageKitUpdater(this))
    , m_refresher(nullptr)
    , m_isFetching(0)
    , m_reviews(OdrsReviewsBackend::global())
{
    QTimer *t = new QTimer(this);
    connect(t, &QTimer::timeout, this, &PackageKitBackend::checkForUpdates);
    t->setInterval(60 * 60 * 1000);
    t->setSingleShot(false);
    t->start();

    connect(&m_details, &Delay::perform, this, &PackageKitBackend::performDetailsFetch);
    connect(&m_details, &Delay::perform, this, [this](const QSet<QString> &pkgids) {
        PackageKit::Transaction *t = PackageKit::Daemon::getUpdatesDetails(kSetToList(pkgids));
        connect(t,
                &PackageKit::Transaction::updateDetail,
                this,
                [this](const QString &packageID,
                       const QStringList &updates,
                       const QStringList &obsoletes,
                       const QStringList &vendorUrls,
                       const QStringList &bugzillaUrls,
                       const QStringList &cveUrls,
                       PackageKit::Transaction::Restart restart,
                       const QString &updateText,
                       const QString &changelog,
                       PackageKit::Transaction::UpdateState state,
                       const QDateTime &issued,
                       const QDateTime &updated) {
                    const QSet<AbstractResource *> resources = resourcesByPackageName(PackageKit::Daemon::packageName(packageID));
                    for (auto r : resources) {
                        PackageKitResource *resource = qobject_cast<PackageKitResource *>(r);
                        if (resource->containsPackageId(packageID)) {
                            resource->updateDetail(packageID,
                                                   updates,
                                                   obsoletes,
                                                   vendorUrls,
                                                   bugzillaUrls,
                                                   cveUrls,
                                                   restart,
                                                   updateText,
                                                   changelog,
                                                   state,
                                                   issued,
                                                   updated);
                        }
                    }
                });
        connect(t, &PackageKit::Transaction::errorCode, this, [this, pkgids](PackageKit::Transaction::Error err, const QString &error) {
            qWarning() << "PackageKitBackend: Error fetching updates:" << err << error;
            for (const QString &pkgid : pkgids) {
                const QSet<AbstractResource *> resources = resourcesByPackageName(PackageKit::Daemon::packageName(pkgid));
                for (auto r : resources) {
                    PackageKitResource *resource = qobject_cast<PackageKitResource *>(r);
                    if (resource->containsPackageId(pkgid)) {
                        Q_EMIT resource->changelogFetched(QString());
                    }
                }
            }
        });
    });

    connect(PackageKit::Daemon::global(), &PackageKit::Daemon::restartScheduled, this, [this] {
        m_updater->setNeedsReboot(true);
    });
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
            else if (!PackageKit::Daemon::global()->offline()->upgradeTriggered())
                fetchUpdates();
            acquireFetching(false);
        },
        this);

    m_globalHints = QStringList() << QStringLiteral("interactive=true") << QStringLiteral("locale=%1").arg(qEnvironmentVariable("LANG"));
    PackageKit::Daemon::global()->setHints(m_globalHints);
}

PackageKitBackend::~PackageKitBackend()
{
    m_threadPool.waitForDone(200);
    m_threadPool.clear();
}

QString proxyFor(KConfigGroup *config, const QString &protocol)
{
    const QString key = protocol + QLatin1String("Proxy");
    QString proxyStr(config->readEntry(key));
    const int index = proxyStr.lastIndexOf(QLatin1Char(' '));

    if (index > -1) {
        const QStringView portStr = QStringView(proxyStr).right(proxyStr.length() - index - 1);
        const bool isDigits = std::all_of(portStr.cbegin(), portStr.cend(), [](const QChar c) {
            return c.isDigit();
        });

        if (isDigits) {
            proxyStr = QStringView(proxyStr).left(index) + QLatin1Char(':') + portStr;
        } else {
            proxyStr.clear();
        }
    }

    return proxyStr;
}

void PackageKitBackend::updateProxy()
{
    if (PackageKit::Daemon::isRunning()) {
        KConfig kioSettings(QStringLiteral("kioslaverc"));
        KConfigGroup proxyConfig = kioSettings.group(u"Proxy Settings"_s);

        bool useProxy = proxyConfig.readEntry<int>("ProxyType", 0) != 0;

        static bool everHad = useProxy;
        if (!everHad && !useProxy)
            return;

        everHad = useProxy;

        PackageKit::Daemon::global()->setProxy(proxyFor(&proxyConfig, QStringLiteral("http")),
                                               proxyFor(&proxyConfig, QStringLiteral("https")),
                                               proxyFor(&proxyConfig, QStringLiteral("ftp")),
                                               proxyFor(&proxyConfig, QStringLiteral("socks")),
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

static bool loadAppStream(AppStream::Pool *appdata)
{
    bool correct = appdata->load();
    if (!correct) {
        qWarning() << "PackageKitBackend: Could not open the AppStream metadata pool" << appdata->lastError();
    }
    return correct;
}

void PackageKitBackend::reloadPackageList()
{
    acquireFetching(true);

    m_appdata.reset(new AppStream::Pool);

    const auto loadDone = [this](bool correct) {
        if (!correct && m_packages.packages.isEmpty()) {
            QTimer::singleShot(0, this, [this]() {
                Q_EMIT passiveMessage(i18n("Please make sure that Appstream is properly set up on your system"));
            });
        }

        // TODO
        // if (data.components.isEmpty()) {
        //     qCDebug(LIBDISCOVER_BACKEND_LOG) << "empty appstream db";
        //     if (PackageKit::Daemon::backendName() == QLatin1String("aptcc") || PackageKit::Daemon::backendName().isEmpty()) {
        //         checkForUpdates();
        //     }
        // }
        if (!m_appstreamInitialized) {
            m_appstreamInitialized = true;
            Q_EMIT loadedAppStream();
        }
        acquireFetching(false);

        const auto distroComponents = m_appdata->componentsById(AppStream::SystemInfo::currentDistroComponentId());
        if (distroComponents.isEmpty()) {
            qWarning() << "PackageKitBackend: No distro component found for" << AppStream::SystemInfo::currentDistroComponentId();
        }
        for (const AppStream::Component &dc : distroComponents) {
            const auto releases = dc.releasesPlain().entries();
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
    };

    // We do not want to block the GUI for very long, so we load appstream pools via queued
    // invocation both for the actual load and then for the post-load state changes.
    QMetaObject::invokeMethod(
        this,
        [this, loadDone] {
            auto result = loadAppStream(m_appdata.get());
            QMetaObject::invokeMethod(
                this,
                [loadDone, result] {
                    loadDone(result);
                },
                Qt::QueuedConnection);
        },
        Qt::QueuedConnection);
}

AppPackageKitResource *PackageKitBackend::addComponent(const AppStream::Component &component) const
{
    const QStringList pkgNames = component.packageNames();
    Q_ASSERT(!pkgNames.isEmpty());
    Q_ASSERT(component.kind() != AppStream::Component::KindFirmware);

    auto appId = makeAppId(component.id());
    AppPackageKitResource *res = qobject_cast<AppPackageKitResource *>(m_packages.packages.value(appId));
    if (!res) {
        res = qobject_cast<AppPackageKitResource *>(m_packagesToAdd.value(appId));
    }
    if (!res) {
        res = new AppPackageKitResource(component, pkgNames.at(0), const_cast<PackageKitBackend *>(this));
        m_packagesToAdd.insert(appId, res);

    }
    for (const QString &pkg : pkgNames) {
        m_packages.packageToApp[pkg] += component.id();
    }
    return res;
}

PKResolveTransaction *PackageKitBackend::resolvePackages(const QStringList &packageNames)
{
    if (packageNames.isEmpty()) {
        return nullptr;
    }

    if (!m_resolveTransaction) {
        m_resolveTransaction = new PKResolveTransaction(this);
        connect(m_resolveTransaction, &PKResolveTransaction::allFinished, this, &PackageKitBackend::getPackagesFinished);
        connect(m_resolveTransaction, &PKResolveTransaction::started, this, [this] {
            m_resolveTransaction = nullptr;
        });
    }

    m_resolveTransaction->addPackageNames(packageNames);
    return m_resolveTransaction;
}

void PackageKitBackend::fetchUpdates()
{
    if (m_updater->isProgressing())
        return;

    PackageKit::Transaction *tUpdates = PackageKit::Daemon::getUpdates();
    connect(tUpdates, &PackageKit::Transaction::finished, this, &PackageKitBackend::getUpdatesFinished);
    connect(tUpdates, &PackageKit::Transaction::package, this, &PackageKitBackend::addPackageToUpdate);
    connect(tUpdates, &PackageKit::Transaction::errorCode, this, &PackageKitBackend::transactionError);
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
        m_packagesToAdd.insert(makePackageId(packageName), pk);
    }
    for (auto res : std::as_const(r))
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
    for (auto [id, res] : KeyValueRange(std::as_const(m_packagesToAdd))) {
        m_packages.packages[id] = res;
    }
    m_packagesToAdd.clear();
    for (PackageKitResource *res : std::as_const(m_packagesToDelete)) {
        const auto pkgs = m_packages.packageToApp.value(res->packageName(), {res->packageName()});
        for (const auto &pkg : pkgs) {
            auto res = m_packages.packages.take(makePackageId(pkg));
            if (res) {
                Q_EMIT resourceRemoved(res);
                res->deleteLater();
            }
        }
    }
    m_packagesToDelete.clear();
    acquireFetching(false);
}

void PackageKitBackend::transactionError(PackageKit::Transaction::Error, const QString &message)
{
    qWarning() << "Transaction error:" << message << sender();
    Q_EMIT passiveMessage(message);
}

void PackageKitBackend::packageDetails(const PackageKit::Details &details)
{
    const QSet<AbstractResource *> resources = resourcesByPackageName(PackageKit::Daemon::packageName(details.packageId()));
    if (resources.isEmpty())
        qWarning() << "PackageKitBackend: Couldn't find package for" << details.packageId();

    for (AbstractResource *res : resources) {
        qobject_cast<PackageKitResource *>(res)->setDetails(details);
    }
}

QSet<AbstractResource *> PackageKitBackend::resourcesByPackageName(const QString &name) const
{
    return resourcesByPackageNames<QSet<AbstractResource *>>(QStringList{name});
}

template<typename T, typename W>
T PackageKitBackend::resourcesByAppNames(const W &appNames) const
{
    T ret;
    ret.reserve(appNames.size());
    for (const QString &name : appNames) {
        AbstractResource *res = m_packages.packages.value(makeAppId(name));
        if (res) {
            ret += res;
        }
    }
    return ret;
}

template<typename T, typename W>
T PackageKitBackend::resourcesByPackageNames(const W &pkgnames) const
{
    T ret;
    ret.reserve(pkgnames.size());
    for (const QString &pkg_name : pkgnames) {
        const QStringList app_names = m_packages.packageToApp.value(pkg_name, QStringList());
        if (app_names.isEmpty()) {
            auto pkgId = makePackageId(pkg_name);
            PackageKitResource *res = qobject_cast<PackageKitResource *>(m_packages.packages.value(pkgId));
            if (!res) {
                res = m_packagesToAdd.value(pkgId);
            }
            if (res) {
                ret += res;
            }
        } else {
            for (const QString &app_id : app_names) {
                const auto appId = makeAppId(app_id);
                AbstractResource *res = m_packages.packages.value(appId);
                if (!res) {
                    res = m_packagesToAdd.value(appId);
                }
                if (res) {
                    ret += res;
                } else {
                    ret += resourcesByComponents<T>(m_appdata->componentsByBundleId(AppStream::Bundle::KindPackage, pkg_name, false));
                }
            }
        }
    }
    return ret;
}

void PackageKitBackend::checkForUpdates()
{
    if (auto offline = PackageKit::Daemon::global()->offline(); offline->updateTriggered() || offline->upgradeTriggered()) {
        qCDebug(LIBDISCOVER_BACKEND_LOG) << "Won't be checking for updates again, the system needs a reboot to apply the fetched offline updates.";
        return;
    }

    if (!m_refresher) {
        acquireFetching(true);
        m_updater->clearDistroUpgrade();
        m_refresher = PackageKit::Daemon::refreshCache(false);
        // Limit the cache-age so that we actually download new caches if necessary
        m_refresher->setHints(globalHints() << QStringLiteral("cache-age=300" /* 5 minutes */));

        connect(m_refresher.data(), &PackageKit::Transaction::errorCode, this, &PackageKitBackend::transactionError);
        connect(m_refresher.data(), &PackageKit::Transaction::percentageChanged, this, &PackageKitBackend::fetchingUpdatesProgressChanged);
        connect(m_refresher.data(), &PackageKit::Transaction::finished, this, [this]() {
            m_refresher = nullptr;
            fetchUpdates();
            acquireFetching(false);
        });
    } else {
        qWarning() << "PackageKitBackend: Already resetting";
    }

    Q_EMIT fetchingUpdatesProgressChanged();
}

AppStream::ComponentBox PackageKitBackend::componentsById(const QString &id) const
{
    Q_ASSERT(m_appstreamInitialized);
    auto comps = m_appdata->componentsById(id);
    if (comps.isEmpty()) {
        comps = m_appdata->componentsByProvided(AppStream::Provided::KindId, id);
    }
    return comps;
}

static const auto needsResolveFilter = [](const StreamResult &res) {
    return res.resource->state() == AbstractResource::Broken;
};

class PKResultsStream : public ResultsStream
{
private:
    PKResultsStream(PackageKitBackend *backend, const QString &name)
        : ResultsStream(name)
        , backend(backend)
    {
    }

    PKResultsStream(PackageKitBackend *backend, const QString &name, const QVector<StreamResult> &resources)
        : ResultsStream(name)
        , backend(backend)
    {
        QTimer::singleShot(0, this, [resources, this]() {
            sendResources(resources);
        });
    }

public:
    template<typename... Args>
    [[nodiscard]] static QPointer<PKResultsStream> create(Args&& ...args)
    {
        return new PKResultsStream(std::forward<Args>(args)...);
    }

    void sendResources(const QVector<StreamResult> &res, bool waitForResolved = false)
    {
        if (res.isEmpty()) {
            finish();
            return;
        }

        Q_ASSERT(res.size() == QSet(res.constBegin(), res.constEnd()).size());
        const auto toResolve = kFilter<QVector<StreamResult>>(res, needsResolveFilter);
        if (!toResolve.isEmpty()) {
            auto transaction = backend->resolvePackages(kTransform<QStringList>(toResolve, [](const StreamResult &res) {
                return res.resource->packageName();
            }));
            if (waitForResolved) {
                Q_ASSERT(transaction);
                connect(transaction, &QObject::destroyed, this, [this, res] {
                    Q_EMIT resourcesFound(res);
                    finish();
                });
                return;
            }
        }

        Q_EMIT resourcesFound(res);
        finish();
    }

private:
    PackageKitBackend *const backend;
};

ResultsStream *PackageKitBackend::search(const AbstractResourcesBackend::Filters &filter)
{
    if (!filter.resourceUrl.isEmpty()) {
        return findResourceByPackageName(filter.resourceUrl);
    } else if (!filter.extends.isEmpty()) {
        auto stream = PKResultsStream::create(this, QStringLiteral("PackageKitStream-extends"));
        auto f = [this, filter, stream] {
            if (!stream) {
                return;
            }
            const auto extendingComponents = m_appdata->componentsByExtends(filter.extends);
            auto resources = resultsByComponents(extendingComponents);
            stream->sendResources(resources, filter.state != AbstractResource::Broken);
        };
        runWhenInitialized(f, stream);
        return stream;
    } else if (filter.state == AbstractResource::Upgradeable) {
        return new ResultsStream(QStringLiteral("PackageKitStream-upgradeable"),
                                 kTransform<QVector<StreamResult>>(upgradeablePackages())); // No need for it to be a PKResultsStream
    } else if (filter.state == AbstractResource::Installed) {
        auto stream = PKResultsStream::create(this, QStringLiteral("PackageKitStream-installed"));
        auto f = [this, stream, filter] {
            if (!stream) {
                return;
            }
            loadAllPackages();
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
                        Q_EMIT stream->resourcesFound(kTransform<QVector<StreamResult>>(resolved, [](auto resource) {
                            return StreamResult{resource, 0};
                        }));
                    stream->finish();
                });
                furtherSearch = true;
            }

            const auto resolved = kFilter<QVector<AbstractResource *>>(m_packages.packages, installedAndNameFilter);
            if (!resolved.isEmpty()) {
                QTimer::singleShot(0, this, [resolved, toResolve, stream]() {
                    if (!resolved.isEmpty())
                        Q_EMIT stream->resourcesFound(kTransform<QVector<StreamResult>>(resolved, [](auto resource) {
                            return StreamResult{resource, 0};
                        }));

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
        auto stream = PKResultsStream::create(this, QStringLiteral("PackageKitStream-all"));
        auto f = [this, filter, stream] {
            if (!stream) {
                return;
            }
            loadAllPackages();
            auto resources = kFilter<QVector<AbstractResource *>>(m_packages.packages, [](AbstractResource *res) {
                return res->type() != AbstractResource::Technical && !qobject_cast<PackageKitResource *>(res)->isCritical()
                    && !qobject_cast<PackageKitResource *>(res)->extendsItself();
            });
            stream->sendResources(kTransform<QVector<StreamResult>>(resources, [](auto resource) {
                return StreamResult{resource, 0};
            }));
        };
        runWhenInitialized(f, stream);
        return stream;
    } else {
        auto stream = PKResultsStream::create(this, QStringLiteral("PackageKitStream-search"));
        const auto f = [this, stream, filter]() {
            if (!stream) {
                return;
            }
            AppStream::ComponentBox components(AppStream::ComponentBox::FlagNone);
            if (!filter.search.isEmpty()) {
                components = m_appdata->search(filter.search);
            } else if (filter.category) {
                components = AppStreamUtils::componentsByCategories(m_appdata.get(), filter.category, AppStream::Bundle::KindUnknown);
            } else {
                components = m_appdata->components();
            }

            QSet<QString> ids;
            kFilterInPlace<AppStream::ComponentBox>(components, [&ids](const AppStream::Component &comp) {
                if (ids.contains(comp.id()))
                    return false;
                ids.insert(comp.id());
                return true;
            });
            if (!ids.isEmpty()) {
                const auto resources = kFilter<QVector<StreamResult>>(resultsByComponents(components), [](const StreamResult &res) {
                    return !qobject_cast<PackageKitResource *>(res.resource)->extendsItself();
                });
                stream->sendResources(resources, filter.state != AbstractResource::Broken);
            } else {
                stream->finish();
            }
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
        QTimer::singleShot(0, stream, f); // NOTE `stream` is a child of `this` so this transitively also depends on `this`
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
            return PKResultsStream::create(this, QStringLiteral("PackageKitStream-localpkg"), QVector<StreamResult>{new LocalFilePKResource(url, this)}).data();
        }
    } else if (url.scheme() == QLatin1String("appstream")) {
        const auto appstreamIds = AppStreamUtils::appstreamIds(url);
        if (appstreamIds.isEmpty())
            Q_EMIT passiveMessage(i18n("Malformed appstream url '%1'", url.toDisplayString()));
        else {
            auto stream = PKResultsStream::create(this, QStringLiteral("PackageKitStream-appstream-url"));
            const auto f = [this, appstreamIds, stream]() {
                if (!stream) {
                    return;
                }
                auto toSend = QSet<StreamResult>();
                toSend.reserve(appstreamIds.size());
                for (const auto &appstreamId : appstreamIds) {
                    const auto comps = componentsById(appstreamId);
                    if (comps.isEmpty()) {
                        continue;
                    }
                    const auto resources = resultsByComponents(comps);
                    for (const auto &r : resources) {
                        toSend.insert(r);
                    }
                }
                stream->sendResources(QList(toSend.constBegin(), toSend.constEnd()));
            };
            runWhenInitialized(f, stream);
            return stream;
        }
    }
    return PKResultsStream::create(this, QStringLiteral("PackageKitStream-unknown-url"), QVector<StreamResult>{}).data();
}

template<typename T>
T PackageKitBackend::resourcesByComponents(const AppStream::ComponentBox &comps) const
{
    T ret;
    ret.reserve(comps.size());
    QSet<QString> done;
    for (const auto &comp : comps) {
        if (comp.packageNames().isEmpty() || done.contains(comp.id())) {
            continue;
        }
        done += comp.id();
        auto r = addComponent(comp);
        Q_ASSERT(r);
        ret << r;
    }
    return ret;
}

QVector<StreamResult> PackageKitBackend::resultsByComponents(const AppStream::ComponentBox &comps) const
{
    QVector<StreamResult> ret;
    ret.reserve(comps.size());
    QSet<QString> done;
    for (const auto &comp : comps) {
        if (comp.packageNames().isEmpty() ||  comp.kind() == AppStream::Component::KindFirmware || done.contains(comp.id())) {
            continue;
        }
        done += comp.id();
        auto r = addComponent(comp);
        Q_ASSERT(r);
        ret << StreamResult{r, comp.sortScore()};
    }
    return ret;
}

bool PackageKitBackend::hasSecurityUpdates() const
{
    return m_hasSecurityUpdates;
}

int PackageKitBackend::updatesCount() const
{
    if (auto offline = PackageKit::Daemon::global()->offline(); offline->updateTriggered() || offline->upgradeTriggered())
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
    for (const QString &pkgid : std::as_const(m_updatesPackageId)) {
        const QString pkgname = PackageKit::Daemon::packageName(pkgid);
        const auto pkgs = resourcesByPackageName(pkgname);
        if (pkgs.isEmpty()) {
            qWarning() << "PackageKitBackend: Couldn't find resource for" << pkgid;
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

    if (!m_updater->isDistroUpgrade() && !PackageKit::Daemon::global()->offline()->upgradeTriggered()) {
        auto nextRelease = AppStreamIntegration::global()->getDistroUpgrade(m_appdata.get());
        if (nextRelease)
            foundNewMajorVersion(*nextRelease);
    }
}

void PackageKitBackend::foundNewMajorVersion(const AppStream::Release &release)
{
    const QString upgradeVersion = release.version();
    const QString newDistroVersionText = AppStreamIntegration::global()->osRelease()->name() + QStringLiteral(" ") + upgradeVersion;

    QString info;
    // Message to display when:
    // - A new major version is available
    // - An update to the current version is available or pending a reboot
    info = i18nc("@info:status %1 is a new major version of the user's distro",
                 "<b>%1 is now available.</b>\n"
                 "To be able to upgrade to this new version, first apply all available updates, and then restart the system.",
                 newDistroVersionText);
    QSharedPointer<InlineMessage> updateBeforeMajorUpgradeMessage =
        QSharedPointer<InlineMessage>::create(InlineMessage::Positive, QStringLiteral("system-software-update"), info);

    // Message to display when:
    // - A new major version is available
    // - No update to the current version are available or pending a reboot
    DiscoverAction *majorUpgrade = new DiscoverAction(QStringLiteral("system-upgrade-symbolic"), i18nc("@action: button", "Upgrade Now"), this);
    connect(majorUpgrade, &DiscoverAction::triggered, this, [this, release, upgradeVersion] {
        if (m_updater->isProgressing())
            return;

        m_updatesPackageId.clear();
        m_updater->setProgressing(true);
        m_refresher = PackageKit::Daemon::upgradeSystem(upgradeVersion,
                                                        PackageKit::Transaction::UpgradeKind::UpgradeKindComplete,
                                                        PackageKit::Transaction::TransactionFlagSimulate);
        m_refresher->setHints(globalHints() << QStringLiteral("cache-age=86400" /* 24*60*60 */));
        connect(m_refresher, &PackageKit::Transaction::package, this, &PackageKitBackend::addPackageToUpdate);
        connect(m_refresher, &PackageKit::Transaction::percentageChanged, this, &PackageKitBackend::fetchingUpdatesProgressChanged);
        connect(m_refresher, &PackageKit::Transaction::errorCode, this, &PackageKitBackend::transactionError);
        connect(m_refresher, &PackageKit::Transaction::finished, this, [this, release](PackageKit::Transaction::Exit e, uint x) {
            m_updater->setDistroUpgrade(release);
            getUpdatesFinished(e, x);
        });
        Q_EMIT inlineMessageChanged({});
        ResourcesModel::global()->switchToUpdates();
    });

    info = i18nc("@info:status %1 is a new major version of the user's distro", "%1 is now available.", newDistroVersionText);
    QSharedPointer<InlineMessage> majorUpgradeAvailableMessage =
        QSharedPointer<InlineMessage>::create(InlineMessage::Positive, QStringLiteral("system-software-update"), info, majorUpgrade);

    // Allow upgrade only if up to date on the current release
    if (!m_updatesPackageId.isEmpty()) {
        Q_EMIT inlineMessageChanged(updateBeforeMajorUpgradeMessage);
        return;
    }

    // No updates pending or avaiable. We are good to offer the upgrade to the
    // next major version!
    Q_EMIT inlineMessageChanged(majorUpgradeAvailableMessage);
}

// Copy of Transaction::packageName that doesn't create a copy but just pass a reference
// It's an optimisation as there's a bunch of allocations that happen from packageName
// Having packageName return a QStringRef or a QStringView would fix this issue.
// TODO Qt 6: Have packageName and similars return a QStringView
static QStringView TransactionpackageName(const QString &packageID)
{
    return QStringView(packageID).left(packageID.indexOf(QLatin1Char(';')));
}

bool PackageKitBackend::isPackageNameUpgradeable(const PackageKitResource *res) const
{
    const QString name = res->packageName();
    for (const QString &pkgid : m_updatesPackageId) {
        if (TransactionpackageName(pkgid) == name)
            return true;
    }
    return false;
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
    m_details.add(pkgid);
}

void PackageKitBackend::performDetailsFetch(const QSet<QString> &pkgids)
{
    Q_ASSERT(!pkgids.isEmpty());
    const auto ids = pkgids.values();

    PackageKit::Transaction *transaction = PackageKit::Daemon::getDetails(ids);
    connect(transaction, &PackageKit::Transaction::details, this, &PackageKitBackend::packageDetails);
    connect(transaction, &PackageKit::Transaction::errorCode, this, &PackageKitBackend::transactionError);
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

QVector<AbstractResource *> PackageKitBackend::extendedBy(const QString &id) const
{
    const auto components = m_appdata->componentsByExtends(id);
    return resourcesByComponents<QVector<AbstractResource *>>(components);
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
    if (!m_refresher)
        return 100;

    int percentage = m_refresher->percentage();
    if (percentage > 100) {
        return m_refresher->property("lastPercentage").toInt();
    }
    m_refresher->setProperty("lastPercentage", percentage);
    return percentage;
}

uint PackageKitBackend::fetchingUpdatesProgressWeight() const
{
    return 50;
}

InlineMessage *PackageKitBackend::explainDysfunction() const
{
    const auto error = m_appdata->lastError();
    if (!error.isEmpty()) {
        return new InlineMessage(InlineMessage::Error, QStringLiteral("network-disconnect"), error);
    }
    return AbstractResourcesBackend::explainDysfunction();
}

void PackageKitBackend::loadAllPackages()
{
    if (m_allPackagesLoaded) {
        return;
    }
    const auto components = m_appdata->components();
    for (const auto &component : components) {
        if (!component.packageNames().isEmpty()) {
            addComponent(component);
        }
    }
    includePackagesToAdd();
    m_allPackagesLoaded = true;
}

#include "PackageKitBackend.moc"
