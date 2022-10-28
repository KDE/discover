/*
 *   SPDX-FileCopyrightText: 2020 Aleix Pol Gonzalez <aleixpol@kde.org>
 *   SPDX-FileCopyrightText: 2021 Mariam Fahmy Sobhy <mariamfahmy66@gmail.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "RpmOstreeBackend.h"
#include "RpmOstreeSourcesBackend.h"

#include "Transaction/TransactionModel.h"

#include <KLocalizedString>

DISCOVER_BACKEND_PLUGIN(RpmOstreeBackend)

Q_DECLARE_METATYPE(QList<QVariantMap>)

static const QString DBusServiceName = QStringLiteral("org.projectatomic.rpmostree1");
static const QString SysrootObjectPath = QStringLiteral("/org/projectatomic/rpmostree1/Sysroot");
static const QString TransactionConnection = QStringLiteral("discover_transaction");

RpmOstreeBackend::RpmOstreeBackend(QObject *parent)
    : AbstractResourcesBackend(parent)
    , m_watcher(new QDBusServiceWatcher(this))
    , m_updater(new StandardBackendUpdater(this))
    , m_fetching(false)
{
    if (!this->isValid()) {
        qWarning() << "rpm-ostree-backend: Was activated even though we are not running on an rpm-ostree managed system";
        return;
    }

    setFetching(true);
    qDBusRegisterMetaType<QList<QVariantMap>>();

    // Setup watcher for rpm-ostree DBus service
    m_watcher->setConnection(QDBusConnection::systemBus());
    m_watcher->addWatchedService(DBusServiceName);
    connect(m_watcher, &QDBusServiceWatcher::serviceOwnerChanged, [this](const QString &serviceName, const QString &oldOwner, const QString &newOwner) {
        qInfo() << "rpm-ostree-backend: Acting on DBus service owner change";

        if (serviceName != DBusServiceName) {
            qWarning() << "rpm-ostree-backend: Got an unexpected event for service:" << serviceName;
            return;
        }
        // Re-activate the service as needed
        if (newOwner.isEmpty()) {
            qWarning() << "rpm-ostree-backend: Got an unexpected unregister event for service:" << serviceName;
            m_dbusActivationTimer->start();
            return;
        }
        if (oldOwner.isEmpty()) {
            initializeBackend();
            return;
        }
        qWarning() << "rpm-ostree-backend: Got an unexpected event for service:" << serviceName << oldOwner << newOwner;
    });

    // Setup timer for activation retries
    m_dbusActivationTimer = new QTimer(this);
    m_dbusActivationTimer->setSingleShot(true);
    m_dbusActivationTimer->setInterval(1000);
    connect(m_dbusActivationTimer, &QTimer::timeout, [this]() {
        QDBusConnection::systemBus().interface()->startService(DBusServiceName);
        qInfo() << "rpm-ostree-backend: DBus activating rpm-ostree service";
    });

    // Look for rpm-ostree's DBus interface among registered services
    const auto reply = QDBusConnection::systemBus().interface()->registeredServiceNames();
    if (!reply.isValid()) {
        qWarning() << "rpm-ostree-backend: Failed to get the list of registered DBus services";
        return;
    }
    const auto registeredServices = reply.value();
    if (registeredServices.contains(DBusServiceName)) {
        // rpm-ostree is running, let's register ourselves as update driver
        initializeBackend();
    } else {
        // Activate the rpm-ostreed daemon via DBus service activation
        QDBusConnection::systemBus().interface()->startService(DBusServiceName);
        qInfo() << "rpm-ostree-backend: DBus activating rpm-ostree service";
    }
}

void RpmOstreeBackend::initializeBackend()
{
    // Connect to the main interface
    m_interface = new OrgProjectatomicRpmostree1SysrootInterface(DBusServiceName, SysrootObjectPath, QDBusConnection::systemBus(), this);
    if (!m_interface->isValid()) {
        qWarning() << "rpm-ostree-backend: Could not connect to rpm-ostree daemon:" << qPrintable(QDBusConnection::systemBus().lastError().message());
        m_dbusActivationTimer->start();
        return;
    }

    // List configured remotes from the system repo and display them in the settings page.
    SourcesModel::global()->addSourcesBackend(new RpmOstreeSourcesBackend(this));

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
        }
    }

    // Fetch the list of refs available on the remote corresponding to the booted deployment
    for (auto deployment : m_resources) {
        if (deployment->isBooted()) {
            deployment->fetchRemoteRefs();
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
        // Look for an existing deployment for the new version
        QVectorIterator<RpmOstreeResource *> iterator(m_resources);
        while (iterator.hasNext()) {
            RpmOstreeResource *deployment = iterator.next();
            if (deployment->version() == newVersion) {
                qInfo() << "rpm-ostree-backend: Found existing deployment for new version. Skipping.";
                return;
            }
        }
        // No existing deployment found. Let's offer the update
        currentlyBootedDeployment()->setNewVersion(newVersion);
        currentlyBootedDeployment()->setState(AbstractResource::Upgradeable);
    });
    TransactionModel::global()->addTransaction(m_transaction);
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
    connect(m_transaction, &RpmOstreeTransaction::deploymentsUpdated, [this]() {
        refreshDeployments();
    });
    return m_transaction;
}

Transaction *RpmOstreeBackend::removeApplication(AbstractResource *)
{
    qWarning() << "rpm-ostree-backend: Unsupported operation:" << __PRETTY_FUNCTION__;
    return nullptr;
}

void RpmOstreeBackend::rebaseToNewVersion(QString ref)
{
    auto curr = currentlyBootedDeployment();
    if (curr->state() == AbstractResource::Upgradeable) {
        qInfo() << "rpm-ostree-backend: Refusing to rebase with pending updates for current version";
        passiveMessage(i18n("Please update to the latest version before rebasing to a major version"));
        return;
    }

    if (ref.isEmpty()) {
        qWarning() << "rpm-ostree-backend: Error: Empty ref to rebase to";
        passiveMessage(i18n("Missing remote ref for rebase operation. Please file a bug."));
        return;
    }

    m_transaction = new RpmOstreeTransaction(this, currentlyBootedDeployment(), m_interface, RpmOstreeTransaction::Rebase, ref);
    connect(m_transaction, &RpmOstreeTransaction::deploymentsUpdated, [this]() {
        refreshDeployments();
    });
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
