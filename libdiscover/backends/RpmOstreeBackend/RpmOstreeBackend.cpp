/*
 *   SPDX-FileCopyrightText: 2020 Aleix Pol Gonzalez <aleixpol@kde.org>
 *   SPDX-FileCopyrightText: 2021 Mariam Fahmy Sobhy <mariamfahmy66@gmail.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "RpmOstreeBackend.h"
#include "RpmOstreeSourcesBackend.h"

DISCOVER_BACKEND_PLUGIN(RpmOstreeBackend)

Q_DECLARE_METATYPE(QList<QVariantMap>)

static const QString DBusServiceName = QStringLiteral("org.projectatomic.rpmostree1");
static const QString SysrootObjectPath = QStringLiteral("/org/projectatomic/rpmostree1/Sysroot");
static const QString TransactionConnection = QStringLiteral("discover_transaction");

RpmOstreeBackend::RpmOstreeBackend(QObject *parent)
    : AbstractResourcesBackend(parent)
    , m_updater(new StandardBackendUpdater(this))
    , m_fetching(false)
{
    if (!this->isValid()) {
        qWarning() << "rpm-ostree-backend: Was activated even though we are not running on an rpm-ostree managed system";
        return;
    }

    setFetching(true);
    qDBusRegisterMetaType<QList<QVariantMap>>();

    // Manually Call 'rpm-ostree status' to ensure the daemon is properly started.
    // TODO: Replace this by proper DBus activation and waiting code.
    QProcess process(this);
    process.start(QStringLiteral("rpm-ostree"), {QStringLiteral("status")});
    process.waitForFinished();

    OrgProjectatomicRpmostree1SysrootInterface interface(DBusServiceName, SysrootObjectPath, QDBusConnection::systemBus(), this);
    if (!interface.isValid()) {
        qWarning() << "rpm-ostree-backend: Could not connect to rpm-ostree daemon:" << qPrintable(QDBusConnection::systemBus().lastError().message());
        return;
    }

    // Registrer ourselves to make sure that rpm-ostreed does not exit while we are running.
    QVariantMap options;
    auto id = QVariant(QStringLiteral("id"));
    options["id"] = "discover";
    interface.RegisterClient(options);

    // Get the path for the curently booted OS DBus interface.
    m_bootedObjectPath = interface.booted().path();

    // List configured remotes from the system repo and display them in the settings page.
    SourcesModel::global()->addSourcesBackend(new RpmOstreeSourcesBackend(this));

    // Get the list of currently available deployments
    QList<QVariantMap> deployments = interface.deployments();
    for (QVariantMap d : deployments) {
        RpmOstreeResource *deployment = new RpmOstreeResource(d, this);
        m_resources << deployment;
        if (deployment->isBooted()) {
            connect(deployment, &RpmOstreeResource::stateChanged, this, &RpmOstreeBackend::updatesCountChanged);
        }
    }

    // Fetch the list of refs available on the remote corresponding to the booted deployment
    for (auto deployment : m_resources) {
        if (deployment->isBooted()) {
            deployment->fetchRemoteRefs();
        }
    }

    connect(m_updater, &StandardBackendUpdater::updatesCountChanged, this, &RpmOstreeBackend::updatesCountChanged);

    // For now, we start fresh: Cancel any in-progress transaction
    // TODO: Do not cancel existing a running transation but show it in the UI
    QString transaction = interface.activeTransactionPath();
    if (!transaction.isEmpty()) {
        qInfo() << "rpm-ostree-backend: A transaction is already in progress";
        QStringList transactionInfo = interface.activeTransaction();
        if (transactionInfo.length() != 3) {
            qInfo() << "rpm-ostree-backend: Unsupported transaction info format:" << transactionInfo;
        } else {
            qInfo() << "rpm-ostree-backend: Operation '" << transactionInfo.at(0) << "' requested by '" << transactionInfo.at(1);
        }
        QDBusConnection peerConnection = QDBusConnection::connectToPeer(transaction, TransactionConnection);
        OrgProjectatomicRpmostree1TransactionInterface transactionInterface(DBusServiceName, QStringLiteral("/"), peerConnection, this);
        qInfo() << "rpm-ostree-backend: Cancelling currently active transaction";
        transactionInterface.Cancel().waitForFinished();
        QDBusConnection::disconnectFromPeer(TransactionConnection);
    }

    // Do not block the UI while we check for updates via rpm-ostree as this
    // can take a while and thus be fustrating for the user
    setFetching(false);

    // Start the check for a new version of the current deployment
    checkForUpdates();
}

RpmOstreeResource *RpmOstreeBackend::currentlyBootedDeployment()
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
    // TODO: Use the code below once we have full Transaction support
    QProcess *process = new QProcess(this);
    connect(process, &QProcess::readyReadStandardError, [process]() {
        qWarning() << "rpm-ostree-backend: Error while calling rpm-ostree:" << process->readAllStandardError();
    });
    connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), [this, process](int exitCode, QProcess::ExitStatus exitStatus) {
        if (exitStatus != QProcess::NormalExit) {
            qWarning() << "rpm-ostree-backend: Error while calling rpm-ostree:" << process->readAllStandardError();
            return;
        }
        if (exitCode != 0) {
            qInfo() << "rpm-ostree-backend: No update available";
            return;
        }
        QString newVersionFound;
        QTextStream stream(process);
        for (QString line = stream.readLine(); stream.readLineInto(&line);) {
            if (line.contains(QLatin1String("Version"))) {
                newVersionFound = line;
            }
        }
        if (!newVersionFound.isEmpty()) {
            newVersionFound.remove(0, QStringLiteral("        Version: ").length() - 1);
            newVersionFound.remove(newVersionFound.size() - QStringLiteral(" (XXXX-XX-XXTXX:XX:XXZ)").length(), newVersionFound.size() - 1);
            currentlyBootedDeployment()->setNewVersion(newVersionFound);
            currentlyBootedDeployment()->setState(AbstractResource::Upgradeable);
        }
        process->deleteLater();
    });
    process->setProcessChannelMode(QProcess::MergedChannels);
    auto prog = QStringLiteral("rpm-ostree");
    auto args = {QStringLiteral("update"), QStringLiteral("--check")};
    process->start(prog, args);

    // OrgProjectatomicRpmostree1OSInterface OSInterface(DBusServiceName, m_bootedObjectPath, QDBusConnection::systemBus(), this);
    // if (!OSInterface.isValid()) {
    //     qWarning() << "rpm-ostree-backend: Could not connect to rpm-ostree daemon:" << qPrintable(QDBusConnection::systemBus().lastError().message());
    //     return;
    // };
    //
    // QVariantMap options;
    // options["mode"] = QVariant(QStringLiteral("check"));
    // options["output-to-self"] = QVariant(false);
    // QVariantMap modifiers;
    //
    // QDBusPendingReply<bool, QString> reply = OSInterface.AutomaticUpdateTrigger(options);
    // reply.waitForFinished();
    // if (reply.isError()) {
    //     qWarning() << "rpm-ostree-backend: Error while calling 'update' in '--check' mode" << reply.error();
    //     return;
    // }
    //
    // m_transaction = new RpmOstreeTransaction(this, currentlyBootedDeployment(), reply.argumentAt<1>());
    // TODO: Register the transaction in the UI
}

int RpmOstreeBackend::updatesCount() const
{
    return m_updater->updatesCount();
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
            // Let's only include the booted deployment until we have better support for multiple deployments
            if (r->isBooted()) {
                res.push_back(r);
            }
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

    OrgProjectatomicRpmostree1OSInterface OSInterface(DBusServiceName, m_bootedObjectPath, QDBusConnection::systemBus(), this);
    if (!OSInterface.isValid()) {
        qWarning() << "rpm-ostree-backend: Could not connect to rpm-ostree daemon:" << qPrintable(QDBusConnection::systemBus().lastError().message());
        return nullptr;
    };

    QVariantMap options;
    QDBusPendingReply<QString> reply = OSInterface.Upgrade(options);
    reply.waitForFinished();
    if (reply.isError()) {
        qWarning() << "rpm-ostree-backend: Error while calling 'update' in '--check' mode" << reply.error();
        return nullptr;
    }

    m_transaction = new RpmOstreeTransaction(this, curr, reply.value());
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

    OrgProjectatomicRpmostree1OSInterface OSInterface(DBusServiceName, m_bootedObjectPath, QDBusConnection::systemBus(), this);
    if (!OSInterface.isValid()) {
        qWarning() << "rpm-ostree-backend: Could not connect to rpm-ostree daemon:" << qPrintable(QDBusConnection::systemBus().lastError().message());
        return;
    };

    QVariantMap options;
    QStringList packages;
    QDBusPendingReply<QString> reply = OSInterface.Rebase(options, ref, packages);
    reply.waitForFinished();
    if (reply.isError()) {
        qWarning() << "rpm-ostree-backend: Error while calling 'update' in '--check' mode" << reply.error();
        return;
    }

    m_transaction = new RpmOstreeTransaction(this, curr, reply.value());
    // TODO: Register the transaction in the UI
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

void RpmOstreeBackend::toggleFetching()
{
    m_fetching = !m_fetching;
    Q_EMIT fetchingChanged();
}

void RpmOstreeBackend::setFetching(bool fetching)
{
    if (m_fetching != fetching) {
        m_fetching = fetching;
        Q_EMIT fetchingChanged();
    }
}

#include "RpmOstreeBackend.moc"
