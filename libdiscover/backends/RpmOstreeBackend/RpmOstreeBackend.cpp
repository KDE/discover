/*
 *   SPDX-FileCopyrightText: 2020 Aleix Pol Gonzalez <aleixpol@kde.org>
 *   SPDX-FileCopyrightText: 2021 Mariam Fahmy Sobhy <mariamfahmy66@gmail.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "RpmOstreeBackend.h"
#include "RpmOstreeResource.h"
#include "RpmOstreeTransaction.h"

#include <QDBusInterface>
#include <QDBusMessage>
#include <QDBusObjectPath>
#include <QDBusPendingReply>
#include <QDebug>
#include <QList>
#include <QMap>
#include <QStringList>
#include <QVariant>
#include <QVariantList>
#include <QFile>
#include <Transaction/Transaction.h>
#include <resources/SourcesModel.h>
#include <resources/StandardBackendUpdater.h>

#include <QStandardItemModel>

DISCOVER_BACKEND_PLUGIN(RpmOstreeBackend)

class RpmOstreeSourcesBackend : public AbstractSourcesBackend
{
public:
    explicit RpmOstreeSourcesBackend(AbstractResourcesBackend *parent)
        : AbstractSourcesBackend(parent)
        , m_model(new QStandardItemModel(this))
    {
        auto it = new QStandardItem(QStringLiteral("rpm-ostree"));
        it->setData(QStringLiteral("rpm-ostree"), IdRole);
        m_model->appendRow(it);
    }

    QAbstractItemModel *sources() override
    {
        return m_model;
    }
    bool addSource(const QString &) override
    {
        return false;
    }
    bool removeSource(const QString &) override
    {
        return false;
    }
    QString idDescription() override
    {
        return QStringLiteral("rpm-ostree");
    }
    QVariantList actions() const override
    {
        return {};
    }

    bool supportsAdding() const override
    {
        return false;
    }
    bool canMoveSources() const override
    {
        return false;
    }

private:
    QStandardItemModel *const m_model;
};

RpmOstreeBackend::RpmOstreeBackend(QObject *parent)
    : AbstractResourcesBackend(parent)
    , m_reviews(AppStreamIntegration::global()->reviews())
    , m_updater(new StandardBackendUpdater(this))
    , m_fetching(true)
    , m_isDeploymentUpdate(true)
{
    connect(m_updater, &StandardBackendUpdater::updatesCountChanged, this, &RpmOstreeBackend::updatesCountChanged);
    getDeployments();
    SourcesModel::global()->addSourcesBackend(new RpmOstreeSourcesBackend(this));
    executeCheckUpdateProcess();
    executeRemoteRefsProcess();
}

void RpmOstreeBackend::getDeployments()
{
    // reading Deployments property from "org.projectatomic.rpmostree1.Sysroot" interface
    QDBusInterface interface(QStringLiteral("org.projectatomic.rpmostree1"),
                             QStringLiteral("/org/projectatomic/rpmostree1/Sysroot"),
                             QStringLiteral("org.freedesktop.DBus.Properties"),
                             QDBusConnection::systemBus());
    QDBusMessage result = interface.call(QStringLiteral("Get"), QStringLiteral("org.projectatomic.rpmostree1.Sysroot"), QStringLiteral("Deployments"));
    QList<QVariant> outArgs = result.arguments();
    QVariant first = outArgs.at(0);
    QDBusVariant dbvFirst = first.value<QDBusVariant>();
    QVariant vFirst = dbvFirst.variant();
    QDBusArgument dbusArgs = vFirst.value<QDBusArgument>();

    // storing the extracted deployments from DBus
    dbusArgs.beginArray();
    while (!dbusArgs.atEnd()) {
        QMap<QString, QVariant> map;
        dbusArgs >> map;
        QDBusArgument dbusArgsSignature = map[QStringLiteral("signatures")].value<QDBusArgument>();
        dbusArgsSignature >> map[QStringLiteral("signatures")];
        QDBusVariant dbvFirst1 = map[QStringLiteral("signatures")].value<QDBusVariant>();
        QVariant vFirst1 = dbvFirst1.variant();
        QDBusArgument dbusArgsSign = vFirst1.value<QDBusArgument>();
        dbusArgsSign >> map[QStringLiteral("signatures")];

        QString baseVersion = map[QStringLiteral("version")].toString();
        QString deploymentName = baseVersion;
        deploymentName.remove(8, deploymentName.size() - 1);
        baseVersion.remove(0, 7);

        // creating a deployment struct
        DeploymentInformation extractDeployment;
        extractDeployment.name = deploymentName;
        extractDeployment.baseVersion = baseVersion;
        extractDeployment.booted = map[QStringLiteral("booted")].toBool();
        extractDeployment.baseChecksum = map[QStringLiteral("checksum")].toString();
        extractDeployment.layeredPackages = map[QStringLiteral("packages")].toString();
        extractDeployment.localPackages = map[QStringLiteral("requested-local-packages")].toString();
        extractDeployment.signature = map[QStringLiteral("signatures")].toString();
        extractDeployment.origin = map[QStringLiteral("origin")].toString();
        extractDeployment.timestamp = map[QStringLiteral("timestamp")].toULongLong();

        m_deployments.push_back(extractDeployment);
    }
    dbusArgs.endArray();

    // create a resource for each deployment
    for (const DeploymentInformation &deployment : m_deployments) {
        QString name = deployment.name;
        QString baseVersion = deployment.baseVersion;
        QString baseChecksum = deployment.baseChecksum;
        QString signature = deployment.signature;
        QString layeredPackages = deployment.layeredPackages;
        QString localPackages = deployment.localPackages;
        QString origin = deployment.origin;
        qulonglong timestamp = deployment.timestamp;
        bool booted = deployment.booted;

        RpmOstreeResource *deploymentResource = new RpmOstreeResource(name, baseVersion, baseChecksum, signature, layeredPackages, localPackages, origin, timestamp, this);

        // changing the state of the booted deployment resource to Installed.
        if (booted) {
            deploymentResource->setState(AbstractResource::Installed);
        }
        m_resources.push_back(deploymentResource);
    }
}

void RpmOstreeBackend::toggleFetching()
{
    m_fetching = !m_fetching;
    checkForUpdatesNeeded();
    emit fetchingChanged();
}

void RpmOstreeBackend::checkForUpdatesNeeded()
{
    if (!m_newVersion.isEmpty()) {
        m_newVersion.remove(0, 25);
        m_newVersion.remove(13, m_newVersion.size() - 13);
        m_resources[0]->setNewVersion(m_newVersion);
        m_resources[0]->setState(AbstractResource::Upgradeable);
        connect(m_resources[0], &RpmOstreeResource::stateChanged, this, &RpmOstreeBackend::updatesCountChanged);
    }
}

void RpmOstreeBackend::executeCheckUpdateProcess()
{
    QProcess *process = new QProcess(this);

    connect(process, &QProcess::readyReadStandardError, [process]() {
        QByteArray readError = process->readAllStandardError();
    });

    // delete process instance when done, and get the exit status to handle errors.
    connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), [this, process](int exitCode, QProcess::ExitStatus exitStatus) {
        qWarning() << "process exited with code " << exitCode << exitStatus;
        if (exitCode == 0) {
            readUpdateOutput(process);
        }
        toggleFetching();
        process->deleteLater();
    });

    process->setProcessChannelMode(QProcess::MergedChannels);
    process->start(QStringLiteral("rpm-ostree"), {QStringLiteral("update"), QStringLiteral("--check")});
}

void RpmOstreeBackend::readUpdateOutput(QIODevice *device)
{
    QTextStream stream(device);
    for (QString line = stream.readLine(); stream.readLineInto(&line);) {
        if (line.contains(QLatin1String("Version"))) {
            m_newVersion = line;
        }
    }
}

void RpmOstreeBackend::executeRemoteRefsProcess()
{
    QProcess *process = new QProcess(this);

    connect(process, &QProcess::readyReadStandardError, [process]() {
        QByteArray readError = process->readAllStandardError();
    });

    // delete process instance when done, and get the exit status to handle errors.
    QObject::connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), [=](int exitCode, QProcess::ExitStatus exitStatus) {
        qWarning() << "process exited with code " << exitCode << exitStatus;
        if (exitCode == 0) {
            readRefsOutput(process);
        }
        process->deleteLater();
    });

    process->setProcessChannelMode(QProcess::MergedChannels);
    process->start(QStringLiteral("ostree"),
                   {QStringLiteral("--repo=/ostree/repo"), QStringLiteral("remote"), QStringLiteral("refs"), QStringLiteral("kinoite")});
}

void RpmOstreeBackend::readRefsOutput(QIODevice *device)
{
    const QString kinoite = QStringLiteral("/kinoite");

    QStringList remoteRefs;
    QTextStream stream(device);
    for (QString ref = stream.readLine(); stream.readLineInto(&ref);) {
        if (ref.endsWith(kinoite))
            continue;
        remoteRefs.push_back(ref);
    }
    m_resources[0]->setRemoteRefsList(remoteRefs);
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
    for (AbstractResource *r : m_resources) {
        if (r->state() >= filter.state)
            res.push_back(r);
    }
    return new ResultsStream(QStringLiteral("rpm-ostree"), res);
}

Transaction *RpmOstreeBackend::installApplication(AbstractResource *app, const AddonList &addons)
{
    updateCurrentDeployment();
    return new RpmOstreeTransaction(qobject_cast<RpmOstreeResource *>(app), addons, Transaction::InstallRole, m_transactionUpdatePath, true);
}

Transaction *RpmOstreeBackend::installApplication(AbstractResource *app)
{
    bool deploymentUpdate = m_isDeploymentUpdate;
    if (m_isDeploymentUpdate) {
        updateCurrentDeployment();
    } else {
        m_isDeploymentUpdate = true;
    }
    return new RpmOstreeTransaction(qobject_cast<RpmOstreeResource *>(app), Transaction::InstallRole, m_transactionUpdatePath, deploymentUpdate);
}

Transaction *RpmOstreeBackend::removeApplication(AbstractResource *)
{
    return nullptr;
}

void RpmOstreeBackend::updateCurrentDeployment()
{ 
    OrgProjectatomicRpmostree1OSInterface interface (QStringLiteral("org.projectatomic.rpmostree1"),
                                                     QStringLiteral("/org/projectatomic/rpmostree1/fedora"),
                                                     QDBusConnection::systemBus(),
                                                     this);
    QVariantMap options;
    QVariantMap modifiers;
    QString name;

    QDBusPendingReply<QString> reply = interface.UpdateDeployment(modifiers, options);
    reply.waitForFinished();
    if (!reply.isError()) {
        m_transactionUpdatePath = reply.argumentAt(0).value<QString>();
    } else {
        qDebug() << "Error occurs when performing the UpdateDeployment: " << reply.error();
    }
}

void RpmOstreeBackend::checkForUpdates()
{
    if (m_fetching)
        return;
    executeCheckUpdateProcess();
    executeRemoteRefsProcess();
    QTimer::singleShot(500, this, &RpmOstreeBackend::toggleFetching);
}

void RpmOstreeBackend::perfromSystemUpgrade(QString selectedRefs)
{
    OrgProjectatomicRpmostree1OSInterface interface (QStringLiteral("org.projectatomic.rpmostree1"),
                                                     QStringLiteral("/org/projectatomic/rpmostree1/fedora"),
                                                     QDBusConnection::systemBus(),
                                                     this);
    m_isDeploymentUpdate = false;
    QVariantMap options;
    QStringList packages;

    QDBusPendingReply<QString> reply = interface.Rebase(options, selectedRefs, packages);
    reply.waitForFinished();
    if (!reply.isError()) {
        m_transactionUpdatePath = reply.argumentAt(0).value<QString>();
        installApplication(m_resources[0]);
    } else {
        qDebug() << "Error occurs when performing the Rebase: " << reply.error();
    }
}

AbstractBackendUpdater *RpmOstreeBackend::backendUpdater() const
{
    return m_updater;
}

AbstractReviewsBackend *RpmOstreeBackend::reviewsBackend() const
{
    return m_reviews.data();
}

QString RpmOstreeBackend::displayName() const
{
    return QStringLiteral("rpm-ostree");
}

bool RpmOstreeBackend::hasApplications() const
{
    return true;
}

#include "RpmOstreeBackend.moc"
