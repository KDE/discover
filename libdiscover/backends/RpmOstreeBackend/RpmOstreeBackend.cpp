/*
 *   SPDX-FileCopyrightText: 2020 Aleix Pol Gonzalez <aleixpol@kde.org>
 *   SPDX-FileCopyrightText: 2021 Mariam Fahmy Sobhy <mariamfahmy66@gmail.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "RpmOstreeBackend.h"
#include "RpmOstreeSourcesBackend.h"

#include "Transaction/TransactionModel.h"

#include <AppStreamQt/release.h>
#include <AppStreamQt/utils.h>
#include <KLocalizedString>
#include <appstream/AppStreamIntegration.h>

DISCOVER_BACKEND_PLUGIN(RpmOstreeBackend)

Q_DECLARE_METATYPE(QList<QVariantMap>)

static const QString DBusServiceName = QStringLiteral("org.projectatomic.rpmostree1");
static const QString SysrootObjectPath = QStringLiteral("/org/projectatomic/rpmostree1/Sysroot");
static const QString TransactionConnection = QStringLiteral("discover_transaction");
static const QString DevelopmentVersionName = QStringLiteral("Rawhide");

RpmOstreeBackend::RpmOstreeBackend(QObject *parent)
    : AbstractResourcesBackend(parent)
    , m_watcher(new QDBusServiceWatcher(this))
    , m_interface(nullptr)
    , m_updater(new StandardBackendUpdater(this))
    , m_fetching(false)
    , m_appdata(new AppStream::Pool)
    , m_developmentEnabled(false)
{
    // Refuse to start on systems not managed by rpm-ostree
    if (!this->isValid()) {
        qWarning() << "rpm-ostree-backend: Not starting on a system not managed by rpm-ostree";
        return;
    }

    // Signal that we're fetching ostree deployments
    setFetching(true);

    // Switch to development branches for testing.
    // TODO: Create a settings option to set this value.
    // m_developmentEnabled = true;

    // List configured remotes and display them in the settings page.
    // We can do this early as this does not depend on the rpm-ostree daemon.
    SourcesModel::global()->addSourcesBackend(new RpmOstreeSourcesBackend(this));

    // Register DBus types
    qDBusRegisterMetaType<QList<QVariantMap>>();

    // Setup watcher for rpm-ostree DBus service
    m_watcher->setConnection(QDBusConnection::systemBus());
    m_watcher->addWatchedService(DBusServiceName);
    connect(m_watcher, &QDBusServiceWatcher::serviceOwnerChanged, [this](const QString &serviceName, const QString &oldOwner, const QString &newOwner) {
        qDebug() << "rpm-ostree-backend: Acting on DBus service owner change";
        if (serviceName != DBusServiceName) {
            // This should never happen
            qWarning() << "rpm-ostree-backend: Got an unexpected event for service:" << serviceName;
        } else if (newOwner.isEmpty()) {
            // Re-activate the service if it goes away unexpectedly
            m_dbusActivationTimer->start();
        } else if (oldOwner.isEmpty()) {
            // This is likely the first activation so let's setup the backend
            initializeBackend();
        } else {
            // This should never happen
            qWarning() << "rpm-ostree-backend: Got an unexpected event for service:" << serviceName << oldOwner << newOwner;
        }
    });

    // Setup timer for activation retries
    m_dbusActivationTimer = new QTimer(this);
    m_dbusActivationTimer->setSingleShot(true);
    m_dbusActivationTimer->setInterval(1000);
    connect(m_dbusActivationTimer, &QTimer::timeout, [this]() {
        QDBusConnection::systemBus().interface()->startService(DBusServiceName);
        qDebug() << "rpm-ostree-backend: DBus activating rpm-ostree service";
    });

    // Look for rpm-ostree's DBus interface among registered services
    const auto reply = QDBusConnection::systemBus().interface()->registeredServiceNames();
    if (!reply.isValid()) {
        qWarning() << "rpm-ostree-backend: Failed to get the list of registered DBus services";
        return;
    }
    const auto registeredServices = reply.value();
    if (registeredServices.contains(DBusServiceName)) {
        // rpm-ostree daemon is running, let's intialize the backend
        initializeBackend();
    } else {
        // Activate the rpm-ostreed daemon via DBus service activation
        QDBusConnection::systemBus().interface()->startService(DBusServiceName);
        qDebug() << "rpm-ostree-backend: DBus activating rpm-ostree service";
    }
}

void RpmOstreeBackend::initializeBackend()
{
    // If any, remove a previous connection that is now likely invalid
    if (m_interface != nullptr) {
        delete m_interface;
    }
    // Connect to the main interface
    m_interface = new OrgProjectatomicRpmostree1SysrootInterface(DBusServiceName, SysrootObjectPath, QDBusConnection::systemBus(), this);
    if (!m_interface->isValid()) {
        qWarning() << "rpm-ostree-backend: Could not connect to rpm-ostree daemon:" << qPrintable(QDBusConnection::systemBus().lastError().message());
        m_dbusActivationTimer->start();
        return;
    }

    // Register ourselves as update driver
    if (!m_registrered) {
        QVariantMap options;
        options["id"] = QVariant{QStringLiteral("discover")};
        auto reply = m_interface->RegisterClient(options);
        QDBusPendingCallWatcher *callWatcher = new QDBusPendingCallWatcher(reply, this);
        connect(callWatcher, &QDBusPendingCallWatcher::finished, [this, callWatcher]() {
            QDBusPendingReply<> reply = *callWatcher;
            callWatcher->deleteLater();
            // Wait and retry if we encounter an error
            if (reply.isError()) {
                qWarning() << "rpm-ostree-backend: Error registering as client:" << qPrintable(QDBusConnection::systemBus().lastError().message());
                m_dbusActivationTimer->start();
            } else {
                // Mark that we are now registered with rpm-ostree and retry
                // initializing the backend
                m_registrered = true;
                initializeBackend();
            }
        });
        return;
    }

    // Fetch existing deployments
    refreshDeployments();

    // Look for a potentially already in-progress rpm-ostree transaction that
    // was started outside of Discover
    const QString transaction = m_interface->activeTransactionPath();
    if (!transaction.isEmpty()) {
        qInfo() << "rpm-ostree-backend: A transaction is already in progress";
        m_transaction = new RpmOstreeTransaction(this, currentlyBootedDeployment(), m_interface, RpmOstreeTransaction::Unknown);
        TransactionModel::global()->addTransaction(m_transaction);
        return;
    }

    // Start the check for a new version of the current deployment if there is
    // no transaction in progress
    checkForUpdates();
}

void RpmOstreeBackend::refreshDeployments()
{
    setFetching(true);

    // Get the path for the curently booted OS DBus interface.
    m_bootedObjectPath = m_interface->booted().path();

    // Reset the list of deployments
    m_resources.clear();

    // Get the list of currently available deployments. This is a DBus property
    // and not a method call so we should not need to do it async.
    const QList<QVariantMap> deployments = m_interface->deployments();
    for (QVariantMap d : deployments) {
        RpmOstreeResource *deployment = new RpmOstreeResource(d, this);
        m_resources << deployment;
        if (deployment->isBooted()) {
            connect(deployment, &RpmOstreeResource::stateChanged, [this]() {
                Q_EMIT updatesCountChanged();
            });
        } else if (deployment->isPending()) {
            // Signal that we have a pending update
            m_updater->enableNeedsReboot();
        }
    }

    // The number of updates might have changed if we're called after an update
    Q_EMIT updatesCountChanged();

    setFetching(false);
}

RpmOstreeResource *RpmOstreeBackend::currentlyBootedDeployment() const
{
    for (RpmOstreeResource *deployment : m_resources) {
        if (deployment->isBooted()) {
            return deployment;
        }
    }
    qWarning() << "rpm-ostree-backend: Requested the currently booted deployment but none found";
    return nullptr;
}

void RpmOstreeBackend::checkForUpdates()
{
    m_transaction = new RpmOstreeTransaction(this, currentlyBootedDeployment(), m_interface, RpmOstreeTransaction::CheckForUpdate);
    connect(m_transaction, &RpmOstreeTransaction::newVersionFound, [this](QString newVersion) {
        // Mark that there is a newer version for the current deployment
        currentlyBootedDeployment()->setNewVersion(newVersion);

        // Look for an existing deployment for the new version
        QVectorIterator<RpmOstreeResource *> iterator(m_resources);
        while (iterator.hasNext()) {
            RpmOstreeResource *deployment = iterator.next();
            if (deployment->version() == newVersion) {
                qInfo() << "rpm-ostree-backend: Found existing deployment for new version. Skipping.";
                // Let the user know that the update is pending a reboot
                m_updater->enableNeedsReboot();
                if (currentlyBootedDeployment()->getNextMajorVersion().isEmpty()) {
                    Q_EMIT inlineMessageChanged(nullptr);
                } else {
                    Q_EMIT inlineMessageChanged(m_rebootBeforeRebaseMessage);
                }
                return;
            }
        }

        // No existing deployment found. Let's offer the update
        currentlyBootedDeployment()->setState(AbstractResource::Upgradeable);
        if (currentlyBootedDeployment()->getNextMajorVersion().isEmpty()) {
            Q_EMIT inlineMessageChanged(nullptr);
        } else {
            Q_EMIT inlineMessageChanged(m_rebootBeforeRebaseMessage);
        }
    });
    connect(m_transaction, &RpmOstreeTransaction::lookForNextMajorVersion, this, &RpmOstreeBackend::lookForNextMajorVersion);
    m_transaction->start();
    TransactionModel::global()->addTransaction(m_transaction);
}

void RpmOstreeBackend::lookForNextMajorVersion()
{
    // Load AppStream metadata
    bool res = m_appdata->load();
    if (!res) {
        qWarning() << "rpm-ostree-backend: Could not open the AppStream metadata pool" << m_appdata->lastError();
        return;
    }

    // Get the DistroComponentId. For Fedora Kinoite, we follow Fedora's
    // release schedule so we don't have our own ID.
    QString distroId = AppStream::Utils::currentDistroComponentId();
    if (distroId == "org.fedoraproject.kinoite.fedora") {
        distroId = "org.fedoraproject.fedora";
    }

    // Look at releases to see if we have a new major version available.
    const QList<AppStream::Component> distroComponents = m_appdata->componentsById(distroId);
    if (distroComponents.isEmpty()) {
        qWarning() << "rpm-ostree-backend: No component found for" << distroId;
        return;
    }

    QString currentVersion = AppStreamIntegration::global()->osRelease()->versionId();
    QString nextVersion;
    for (const AppStream::Component &dc : distroComponents) {
        const auto releases = dc.releases();
        for (const auto &r : releases) {
            // Only look at stable releases unless development mode is enabled
            if ((r.kind() != AppStream::Release::KindStable) && !m_developmentEnabled) {
                continue;
            }

            // Let's look at this potentially new verson
            QString newVersion = r.version();

            // Ignore development versions by default for version comparisions.
            // With the development mode enabled, we will offer it later if no
            // other previous newer major version is found.
            if (newVersion == DevelopmentVersionName) {
                continue;
            }

            if (AppStream::Utils::vercmpSimple(newVersion, currentVersion) > 0) {
                if (nextVersion.isEmpty()) {
                    // No other newer version found yet so let's pick this one
                    nextVersion = newVersion;
                    qInfo() << "rpm-ostree-backend: Found new major release:" << nextVersion;
                } else if (AppStream::Utils::vercmpSimple(nextVersion, newVersion) > 0) {
                    // We only offer updating to the very next major release so
                    // we pick the smallest of all the newest versions
                    nextVersion = newVersion;
                    qInfo() << "rpm-ostree-backend: Found a closer new major release:" << nextVersion;
                }
            }
        }
    }
    if (!nextVersion.isEmpty()) {
        // Offer the newly found major version to rebase to
        foundNewMajorVersion(nextVersion);
    } else if (m_developmentEnabled) {
        // If development mode is enabled, always offer Rawhide as an option if
        // we are already on the latest major version.
        foundNewMajorVersion(DevelopmentVersionName);
    }
}

void RpmOstreeBackend::foundNewMajorVersion(const QString &newMajorVersion)
{
    qDebug() << "rpm-ostree-backend: Found new release:" << newMajorVersion;

    QString info;
    // Message to display when:
    // - A new major version is available
    // - An update to the current version is available or pending a reboot
    info = i18n(
        "<b>A new major version of %1 has been released!</b>\n"
        "To be able to update to this new version, make sure to apply all pending updates and reboot your system.",
        currentlyBootedDeployment()->packageName());
    m_rebootBeforeRebaseMessage = QSharedPointer<InlineMessage>::create(InlineMessage::Positive, QStringLiteral("application-x-rpm"), info);

    // Message to display when:
    // - A new major version is available
    // - No update to the current version are available or pending a reboot
    DiscoverAction *rebase = new DiscoverAction(i18n("Upgrade to %1 %2", currentlyBootedDeployment()->packageName(), newMajorVersion), this);
    connect(rebase, &DiscoverAction::triggered, this, &RpmOstreeBackend::rebaseToNewVersion);
    info = i18n("<b>A new major version has been released!</b>");
    m_rebaseAvailableMessage = QSharedPointer<InlineMessage>::create(InlineMessage::Positive, QStringLiteral("application-x-rpm"), info, rebase);

    // Look for an existing deployment for the new major version
    QVectorIterator<RpmOstreeResource *> iterator(m_resources);
    while (iterator.hasNext()) {
        RpmOstreeResource *deployment = iterator.next();
        QString deploymentVersion = deployment->version();
        QStringList deploymentVersionSplit = deploymentVersion.split('.');
        if (!deploymentVersionSplit.empty()) {
            deploymentVersion = deploymentVersionSplit.at(0);
        }
        if (deploymentVersion == newMajorVersion) {
            qInfo() << "rpm-ostree-backend: Found existing deployment for new major version";
            m_updater->enableNeedsReboot();
            Q_EMIT inlineMessageChanged(nullptr);
            return;
        }
    }

    // Look for an existing updated deployment or a pending deployment for the
    // current version
    QString newVersion = currentlyBootedDeployment()->getNewVersion();
    iterator = QVectorIterator<RpmOstreeResource *>(m_resources);
    while (iterator.hasNext()) {
        RpmOstreeResource *deployment = iterator.next();
        if ((deployment->version() == newVersion) || deployment->isPending()) {
            qInfo() << "rpm-ostree-backend: Found pending or updated deployment for current version";
            m_updater->enableNeedsReboot();
            Q_EMIT inlineMessageChanged(m_rebootBeforeRebaseMessage);
            return;
        }
    }

    // No pending deployment found for the current version. We effectively let
    // them upgrade only if they are running the latest version of the current
    // release so let's check if there is an update available for the current
    // version.
    if (currentlyBootedDeployment()->state() == AbstractResource::Upgradeable) {
        qInfo() << "rpm-ostree-backend: Found pending update for current version";
        m_updater->enableNeedsReboot();
        Q_EMIT inlineMessageChanged(m_rebootBeforeRebaseMessage);
        return;
    }

    // No updates pending or avaiable. We are good to offer the rebase to the
    // next major version!
    Q_EMIT inlineMessageChanged(m_rebaseAvailableMessage);
    // Finally set the new major version for the resource only when we are truly
    // ready to offer to rebase to it
    currentlyBootedDeployment()->setNewMajorVersion(newMajorVersion);
}

int RpmOstreeBackend::updatesCount() const
{
    auto curr = currentlyBootedDeployment();
    if (curr->state() == AbstractResource::Upgradeable) {
        return 1;
    }
    return 0;
}

bool RpmOstreeBackend::isValid() const
{
    return QFile::exists(QStringLiteral("/run/ostree-booted"));
}

ResultsStream *RpmOstreeBackend::search(const AbstractResourcesBackend::Filters &filter)
{
    QVector<AbstractResource *> res;
    for (RpmOstreeResource *r : m_resources) {
        if (r->state() >= filter.state) {
            res << r;
        }
    }
    return new ResultsStream(QStringLiteral("rpm-ostree"), res);
}

Transaction *RpmOstreeBackend::installApplication(AbstractResource *app, const AddonList &addons)
{
    Q_UNUSED(addons);
    return installApplication(app);
}

Transaction *RpmOstreeBackend::installApplication(AbstractResource *app)
{
    Q_UNUSED(app);
    auto curr = currentlyBootedDeployment();
    if (curr->state() != AbstractResource::Upgradeable) {
        return nullptr;
    }

    m_transaction = new RpmOstreeTransaction(this, currentlyBootedDeployment(), m_interface, RpmOstreeTransaction::Update);
    connect(m_transaction, &RpmOstreeTransaction::deploymentsUpdated, this, &RpmOstreeBackend::refreshDeployments);
    m_transaction->start();
    return m_transaction;
}

Transaction *RpmOstreeBackend::removeApplication(AbstractResource *)
{
    // TODO: Support removing unbooted & unpinned deployments
    qWarning() << "rpm-ostree-backend: Unsupported operation:" << __PRETTY_FUNCTION__;
    return nullptr;
}

void RpmOstreeBackend::rebaseToNewVersion()
{
    auto curr = currentlyBootedDeployment();
    if (curr->state() == AbstractResource::Upgradeable) {
        if (m_developmentEnabled) {
            qInfo() << "rpm-ostree-backend: You have pending updates for current version. Proceeding anyway.";
            passiveMessage(i18n("You have pending updates for the current version. Proceeding anyway."));
        } else {
            qInfo() << "rpm-ostree-backend: Refusing to rebase with pending updates for current version";
            passiveMessage(i18n("Please update to the latest version before rebasing to a major version"));
            return;
        }
    }

    const QString ref = curr->getNextMajorVersionRef();
    if (ref.isEmpty()) {
        qWarning() << "rpm-ostree-backend: Error: Empty ref to rebase to";
        passiveMessage(i18n("Missing remote ref for rebase operation. Please file a bug."));
        return;
    }

    // Only start one rebase operation at a time
    Q_EMIT inlineMessageChanged(nullptr);
    m_transaction = new RpmOstreeTransaction(this, currentlyBootedDeployment(), m_interface, RpmOstreeTransaction::Rebase, ref);
    connect(m_transaction, &RpmOstreeTransaction::deploymentsUpdated, this, &RpmOstreeBackend::refreshDeployments);
    connect(m_transaction, &RpmOstreeTransaction::lookForNextMajorVersion, this, &RpmOstreeBackend::lookForNextMajorVersion);
    m_transaction->start();
    TransactionModel::global()->addTransaction(m_transaction);
}

AbstractBackendUpdater *RpmOstreeBackend::backendUpdater() const
{
    return m_updater;
}

QString RpmOstreeBackend::displayName() const
{
    return QStringLiteral("rpm-ostree");
}

bool RpmOstreeBackend::hasApplications() const
{
    return true;
}

AbstractReviewsBackend *RpmOstreeBackend::reviewsBackend() const
{
    return nullptr;
}

bool RpmOstreeBackend::isFetching() const
{
    return m_fetching;
}

void RpmOstreeBackend::setFetching(bool fetching)
{
    if (m_fetching != fetching) {
        m_fetching = fetching;
        Q_EMIT fetchingChanged();
    }
}

#include "RpmOstreeBackend.moc"
