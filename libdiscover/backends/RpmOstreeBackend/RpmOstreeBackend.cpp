/*
 *   SPDX-FileCopyrightText: 2020 Aleix Pol Gonzalez <aleixpol@kde.org>
 *   SPDX-FileCopyrightText: 2021 Mariam Fahmy Sobhy <mariamfahmy66@gmail.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "RpmOstreeBackend.h"
#include "RpmOstreeSourcesBackend.h"

#include "Category/Category.h"
#include "Transaction/TransactionModel.h"

#include <AppStreamQt/release.h>
#include <AppStreamQt/systeminfo.h>
#include <AppStreamQt/utils.h>
#include <KLocalizedString>
#include <appstream/AppStreamIntegration.h>

DISCOVER_BACKEND_PLUGIN(RpmOstreeBackend)

Q_DECLARE_METATYPE(QList<QVariantMap>)

static const QString DBusServiceName = QStringLiteral("org.projectatomic.rpmostree1");
static const QString SysrootObjectPath = QStringLiteral("/org/projectatomic/rpmostree1/Sysroot");
static const QString TransactionConnection = QStringLiteral("discover_transaction");

RpmOstreeBackend::RpmOstreeBackend(QObject *parent)
    : AbstractResourcesBackend(parent)
    , m_currentlyBootedDeployment(nullptr)
    , m_transaction(nullptr)
    , m_watcher(new QDBusServiceWatcher(this))
    , m_interface(nullptr)
    , m_updater(new StandardBackendUpdater(this))
    , m_fetching(false)
    , m_appdata(new AppStream::Pool)
{
    // Refuse to start on systems not managed by rpm-ostree
    if (!this->isValid()) {
        qWarning() << "rpm-ostree-backend: Not starting on a system not managed by rpm-ostree";
        return;
    }

    // Signal that we're fetching ostree deployments
    setFetching(true);

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
        options[QLatin1String("id")] = QVariant{QStringLiteral("discover")};
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
    if (hasExternalTransaction()) {
        setFetching(false);
        return;
    }

    // Start the check for a new version of the current deployment if there is
    // no transaction in progress
    checkForUpdates();
}

void RpmOstreeBackend::refreshDeployments()
{
    // Set fetching in all cases but record the state to decide if we should undo it at the end
    bool wasFetching = isFetching();
    setFetching(true);

    // Get the path for the curently booted OS DBus interface.
    m_bootedObjectPath = m_interface->booted().path();

    // Reset the list of deployments
    m_currentlyBootedDeployment = nullptr;
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
            if (m_currentlyBootedDeployment) {
                qWarning() << "rpm-ostree-backend: We already have a booted deployment. This is a bug.";
                passiveMessage(i18n("rpm-ostree: Multiple booted deployments found. Please file a bug."));
                return;
            }
            m_currentlyBootedDeployment = deployment;
        } else if (deployment->isPending()) {
            // Signal that we have a pending update
            m_updater->setNeedsReboot(true);
        }
    }

    if (!m_currentlyBootedDeployment) {
        qWarning() << "rpm-ostree-backend: We have not found the booted deployment. This is a bug.";
        passiveMessage(i18n("rpm-ostree: No booted deployment found. Please file a bug."));
        return;
    }

    // The number of updates might have changed if we're called after an update
    Q_EMIT updatesCountChanged();

    // Only undo the fetching state if we set it up in this function
    if (!wasFetching) {
        setFetching(false);
    }
}

void RpmOstreeBackend::transactionStatusChanged(Transaction::Status status)
{
    switch (status) {
    case Transaction::Status::DoneStatus:
    case Transaction::Status::DoneWithErrorStatus:
    case Transaction::Status::CancelledStatus:
        m_transaction = nullptr;
        setFetching(false);
        break;
    default:
        // Ignore all other status changes
        ;
    }
}

void RpmOstreeBackend::setupTransaction(RpmOstreeTransaction::Operation op, QString arg)
{
    m_transaction = new RpmOstreeTransaction(this, m_currentlyBootedDeployment, m_interface, op, arg);
    connect(m_transaction, &RpmOstreeTransaction::statusChanged, this, &RpmOstreeBackend::transactionStatusChanged);
    connect(m_transaction, &RpmOstreeTransaction::deploymentsUpdated, this, &RpmOstreeBackend::refreshDeployments);
    connect(m_transaction, &RpmOstreeTransaction::lookForNextMajorVersion, this, &RpmOstreeBackend::lookForNextMajorVersion);
}

bool RpmOstreeBackend::hasExternalTransaction()
{
    // Do we already know that we have a transaction in progress?
    if (m_transaction) {
        qInfo() << "rpm-ostree-backend: A transaction is already in progress";
        return true;
    }

    // Is there actualy a transaction in progress we don't know about yet?
    const QString transaction = m_interface->activeTransactionPath();
    if (!transaction.isEmpty()) {
        qInfo() << "rpm-ostree-backend: Found a transaction in progress";
        // We don't check that m_currentlyBootedDeployment is != nullptr here as we expect
        // that the backend is initialized when we're called.
        setupTransaction(RpmOstreeTransaction::Unknown);
        TransactionModel::global()->addTransaction(m_transaction);
        return true;
    }

    return false;
}

void RpmOstreeBackend::checkForUpdates()
{
    if (!m_currentlyBootedDeployment) {
        // Should never happen
        qWarning() << "rpm-ostree-backend: Called checkForUpdates before the backend is done getting deployments. File a bug to your distribution.";
        return;
    }

    // Do not start a transaction if there is already one in-progress (likely externaly started)
    if (hasExternalTransaction()) {
        qInfo() << "rpm-ostree-backend: Not checking for updates while a transaction is in progress";
        return;
    }

    // We're fetching updates
    setFetching(true);

    setupTransaction(RpmOstreeTransaction::CheckForUpdate);
    connect(m_transaction, &RpmOstreeTransaction::newVersionFound, [this](QString newVersion) {
        // Mark that there is a newer version for the current deployment
        m_currentlyBootedDeployment->setNewVersion(newVersion);

        // Look for an existing deployment for the new version
        QVectorIterator<RpmOstreeResource *> iterator(m_resources);
        while (iterator.hasNext()) {
            RpmOstreeResource *deployment = iterator.next();
            if (deployment->version() == newVersion) {
                qInfo() << "rpm-ostree-backend: Found existing deployment for new version. Skipping.";
                // Let the user know that the update is pending a reboot
                m_updater->setNeedsReboot(true);
                if (m_currentlyBootedDeployment->getNextMajorVersion().isEmpty()) {
                    Q_EMIT inlineMessageChanged(nullptr);
                } else {
                    Q_EMIT inlineMessageChanged(m_rebootBeforeRebaseMessage);
                }
                return;
            }
        }

        // No existing deployment found. Let's offer the update
        m_currentlyBootedDeployment->setState(AbstractResource::Upgradeable);
        if (m_currentlyBootedDeployment->getNextMajorVersion().isEmpty()) {
            Q_EMIT inlineMessageChanged(nullptr);
        } else {
            Q_EMIT inlineMessageChanged(m_rebootBeforeRebaseMessage);
        }
    });
    m_transaction->start();
    TransactionModel::global()->addTransaction(m_transaction);
}

void RpmOstreeBackend::lookForNextMajorVersion()
{
    qInfo() << "rpm-ostree-backend: Looking for a new major version";

    const auto loadDone = [this](bool correct) {
        if (!correct) {
            qWarning() << "rpm-ostree-backend: Could not open the AppStream metadata pool" << m_appdata->lastError();
            return;
        }

        // Get the DistroComponentId. For Fedora Kinoite, we follow Fedora's
        // release schedule so we don't have our own ID.
        QString distroId = AppStream::SystemInfo::currentDistroComponentId();
        if (distroId == QLatin1String("org.fedoraproject.kinoite.fedora")) {
            distroId = QStringLiteral("org.fedoraproject.fedora");
        }

        auto nextRelease = AppStreamIntegration::global()->getDistroUpgrade(m_appdata.get(), distroId);
        if (nextRelease) {
            if (!m_currentlyBootedDeployment) {
                // Should never happen
                qWarning()
                    << "rpm-ostree-backend: Called lookForNextMajorVersion before the backend is done getting deployments. File a bug to your distribution.";
                return;
            }
            // Validate that the branch exists for the version to move to and set it for the resource
            if (!m_currentlyBootedDeployment->setNewMajorVersion(nextRelease->version())) {
                qInfo() << "rpm-ostree-backend: Not offering new version:" << nextRelease->version();
                return;
            }
            // Offer the newly found major version to rebase to
            foundNewMajorVersion(*nextRelease);
            return;
        }

        qInfo() << "rpm-ostree-backend: No new major version found";
    };

    connect(m_appdata.get(), &AppStream::Pool::loadFinished, this, loadDone);
    m_appdata->loadAsync();
}

void RpmOstreeBackend::foundNewMajorVersion(const AppStream::Release &release)
{
    QString newMajorVersion = release.version();
    if (newMajorVersion == QStringLiteral("rawhide")) {
        newMajorVersion = QStringLiteral("Rawhide");
    }
    qDebug() << "rpm-ostree-backend: Found new release:" << newMajorVersion;

    if (!m_currentlyBootedDeployment) {
        // Should never happen
        qWarning() << "rpm-ostree-backend: Called foundNewMajorVersion before the backend is done getting deployments. File a bug to your distribution.";
        return;
    }
    const QString newDistroVersionText = m_currentlyBootedDeployment->packageName() + QStringLiteral(" ") + newMajorVersion;

    QString info;
    // Message to display when:
    // - A new major version is available
    // - An update to the current version is available or pending a reboot
    info = i18nc("@info:status %1 is a new major version of the user's distro",
                 "<b>%1 is now available.</b>\n"
                 "To be able to upgrade to this new version, first apply all available updates, and then restart the system.",
                 newDistroVersionText);
    m_rebootBeforeRebaseMessage = QSharedPointer<InlineMessage>::create(InlineMessage::Positive, QStringLiteral("system-software-update"), info);

    // Message to display when:
    // - A new major version is available
    // - No update to the current version are available or pending a reboot
    DiscoverAction *majorUpgrade = new DiscoverAction(QStringLiteral("system-upgrade-symbolic"), i18nc("@action: button", "Begin Upgrade…"), this);
    connect(majorUpgrade, &DiscoverAction::triggered, this, &RpmOstreeBackend::rebaseToNewVersion);
    info = i18nc("@info:status %1 is a new major version of the user's distro", "%1 is now available.", newDistroVersionText);
    m_rebaseAvailableMessage = QSharedPointer<InlineMessage>::create(InlineMessage::Positive, QStringLiteral("system-software-update"), info, majorUpgrade);

    // Look for an existing deployment for the new major version
    QVectorIterator<RpmOstreeResource *> iterator(m_resources);
    while (iterator.hasNext()) {
        RpmOstreeResource *deployment = iterator.next();
        QString deploymentVersion = deployment->version();
        QStringList deploymentVersionSplit = deploymentVersion.split(QLatin1Char('.'));
        if (!deploymentVersionSplit.empty()) {
            deploymentVersion = deploymentVersionSplit.at(0);
        }
        if (deploymentVersion == newMajorVersion) {
            qInfo() << "rpm-ostree-backend: Found existing deployment for new major version";
            m_updater->setNeedsReboot(true);
            Q_EMIT inlineMessageChanged(nullptr);
            return;
        }
    }

    // Hidden environement variable to help debugging rebases, skipping the checks below
    if (qEnvironmentVariableIntValue("DISCOVER_RPM_OSTREE_DEVEL") != 0) {
        Q_EMIT inlineMessageChanged(m_rebaseAvailableMessage);
        return;
    }

    // Look for an existing updated deployment or a pending deployment for the
    // current version
    QString newVersion = m_currentlyBootedDeployment->availableVersion();
    iterator = QVectorIterator<RpmOstreeResource *>(m_resources);
    while (iterator.hasNext()) {
        RpmOstreeResource *deployment = iterator.next();
        if ((deployment->version() == newVersion) || deployment->isPending()) {
            qInfo() << "rpm-ostree-backend: Found pending or updated deployment for current version";
            m_updater->setNeedsReboot(true);
            Q_EMIT inlineMessageChanged(m_rebootBeforeRebaseMessage);
            return;
        }
    }

    // No pending deployment found for the current version. We effectively let
    // them upgrade only if they are running the latest version of the current
    // release so let's check if there is an update available for the current
    // version.
    if (m_currentlyBootedDeployment->state() == AbstractResource::Upgradeable) {
        qInfo() << "rpm-ostree-backend: Found pending update for current version";
        m_updater->setNeedsReboot(true);
        Q_EMIT inlineMessageChanged(m_rebootBeforeRebaseMessage);
        return;
    }

    // No updates pending or avaiable. We are good to offer the rebase to the
    // next major version!
    Q_EMIT inlineMessageChanged(m_rebaseAvailableMessage);
}

int RpmOstreeBackend::updatesCount() const
{
    if (!m_currentlyBootedDeployment) {
        // Not yet initialized
        return 0;
    }
    if (m_currentlyBootedDeployment->state() == AbstractResource::Upgradeable) {
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
    // Skip the search if we're looking into a Category, but not the "Operating System" category
    if (filter.category && filter.category->untranslatedName() != QLatin1String("Operating System")) {
        return new ResultsStream(QStringLiteral("rpm-ostree-empty"), {});
    }

    // Trim whitespace from beginning and end of the string entered in the search field.
    QString keyword = filter.search.trimmed();

    QVector<StreamResult> res;
    for (RpmOstreeResource *r : m_resources) {
        // Skip if the state does not match the filter
        if (r->state() < filter.state) {
            continue;
        }
        // Skip if the search field is not empty and neither the name, description or version matches
        if (!keyword.isEmpty()) {
            if (!r->name().contains(keyword) && !r->longDescription().contains(keyword) && !r->installedVersion().contains(keyword)) {
                continue;
            }
        }
        // Add the ressources to the search filter
        res << r;
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

    if (!m_currentlyBootedDeployment) {
        qInfo() << "rpm-ostree-backend: Called installApplication before the backend is done getting deployments";
        return nullptr;
    }

    if (m_currentlyBootedDeployment->state() != AbstractResource::Upgradeable) {
        return nullptr;
    }

    setupTransaction(RpmOstreeTransaction::Update);
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
    if (!m_currentlyBootedDeployment) {
        qInfo() << "rpm-ostree-backend: Called rebaseToNewVersion before the backend is done getting deployments";
        return;
    }

    if (m_currentlyBootedDeployment->state() == AbstractResource::Upgradeable) {
        // Hidden environement variable to help debugging this path
        if (qEnvironmentVariableIntValue("DISCOVER_RPM_OSTREE_DEVEL") != 0) {
            qInfo() << "rpm-ostree-backend: You have pending updates for current version. Proceeding anyway.";
            passiveMessage(i18n("You have pending updates for the current version. Proceeding anyway."));
        } else {
            qInfo() << "rpm-ostree-backend: Refusing to rebase with pending updates for current version";
            passiveMessage(i18n("Please update to the latest version before rebasing to a major version"));
            return;
        }
    }

    const QString ref = m_currentlyBootedDeployment->getNextMajorVersionRef();
    if (ref.isEmpty()) {
        qWarning() << "rpm-ostree-backend: Error: Empty ref to rebase to";
        passiveMessage(i18n("Missing remote ref for rebase operation. Please file a bug."));
        return;
    }

    // Only start one rebase operation at a time
    Q_EMIT inlineMessageChanged(nullptr);
    setupTransaction(RpmOstreeTransaction::Rebase, ref);
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
#include "moc_RpmOstreeBackend.cpp"
