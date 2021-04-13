/*
 *   SPDX-FileCopyrightText: 2020 Aleix Pol Gonzalez <aleixpol@kde.org>
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
        auto it = new QStandardItem(QStringLiteral("RpmOstree"));
        it->setData(QStringLiteral("RpmOstree"), IdRole);
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
        return QStringLiteral("RpmOstree");
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
    , isDeploymentUpdate(true)
    , m_fetching(true)
    , m_reviews(AppStreamIntegration::global()->reviews())
    , m_updater(new StandardBackendUpdater(this))
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
        deploymentInformation extractDeployment;
        extractDeployment.name = deploymentName;
        extractDeployment.baseVersion = baseVersion;
        extractDeployment.booted = map[QStringLiteral("booted")].toBool();
        extractDeployment.baseChecksum = map[QStringLiteral("checksum")].toString();
        extractDeployment.layeredPackages = map[QStringLiteral("packages")].toString();
        extractDeployment.localPackages = map[QStringLiteral("requested-local-packages")].toString();
        extractDeployment.signature = map[QStringLiteral("signatures")].toString();
        extractDeployment.origin = map[QStringLiteral("origin")].toString();

        deploymentsList.push_back(extractDeployment);
    }
    dbusArgs.endArray();

    // create a resource for each deployment
    for (const deploymentInformation &deployment : deploymentsList) {
        QString name = deployment.name;
        QString baseVersion = deployment.baseVersion;
        QString baseChecksum = deployment.baseChecksum;
        QString signature = deployment.signature;
        QString layeredPackages = deployment.layeredPackages;
        QString localPackages = deployment.localPackages;
        QString origin = deployment.origin;
        bool booted = deployment.booted;

        RpmOstreeResource *deploymentResource = new RpmOstreeResource(name, baseVersion, baseChecksum, signature, layeredPackages, localPackages, origin, this);

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
    QProcess *process = new QProcess();

    connect(process, &QProcess::readyReadStandardError, [process]() {
        QByteArray readError = process->readAllStandardError();
    });

    // catch data output
    connect(process, &QProcess::readyReadStandardOutput, this, [process, this]() {
        QByteArray readOutput = process->readAllStandardOutput();
        this->getQProcessUpdateOutput(readOutput);
    });

    // delete process instance when done, and get the exit status to handle errors.
    connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), [=](int exitCode, QProcess::ExitStatus exitStatus) {
        qWarning() << "process exited with code " << exitCode;
        toggleFetching();
        process->deleteLater();
    });

    process->setProcessChannelMode(QProcess::MergedChannels);
    process->start(QStringLiteral("rpm-ostree update --check"));
}

void RpmOstreeBackend::getQProcessUpdateOutput(QByteArray readOutput)
{
    QList<QByteArray> checkUpdateOutput = readOutput.split('\n');

    for (const QByteArray &output : checkUpdateOutput) {
        if (output.contains(QByteArray("Version"))) {
            m_newVersion = QString::fromLocal8Bit(output);
        }
    }
}

void RpmOstreeBackend::executeRemoteRefsProcess()
{
    if (!m_remoteRefsList.isEmpty())
        m_remoteRefsList.clear();

    QProcess *process = new QProcess(this);

    connect(process, &QProcess::readyReadStandardError, [process]() {
        QByteArray readError = process->readAllStandardError();
    });

    // catch data output
    connect(process, &QProcess::readyReadStandardOutput, this, [process, this]() {
        QByteArray readOutput = process->readAllStandardOutput();
        this->getQProcessRefsOutput(readOutput);
    });

    // delete process instance when done, and get the exit status to handle errors.
    QObject::connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), [=](int exitCode, QProcess::ExitStatus exitStatus) {
        qWarning() << "process exited with code " << exitCode;
        settingRemoteRefsDeploymentResource();
        process->deleteLater();
    });

    process->setProcessChannelMode(QProcess::MergedChannels);
    process->start(QStringLiteral("ostree --repo=/ostree/repo remote refs kinoite"));
}

void RpmOstreeBackend::getQProcessRefsOutput(QByteArray readOutput)
{
    QList<QByteArray> remoteRefsList;
    remoteRefsList = readOutput.split('\n');

    QString kinoite = QStringLiteral("kinoite");

    for (const QByteArray &refs : remoteRefsList) {
        QString refsToString = QString::fromLocal8Bit(refs);
        if (refsToString.count(kinoite) <= 1)
            continue;
        m_remoteRefsList.push_back(refsToString);
    }
}

void RpmOstreeBackend::settingRemoteRefsDeploymentResource()
{
    m_resources[0]->setRemoteRefsList(m_remoteRefsList);
}

int RpmOstreeBackend::updatesCount() const
{
    return m_updater->updatesCount();
}

ResultsStream *RpmOstreeBackend::search(const AbstractResourcesBackend::Filters &filter)
{
    QVector<AbstractResource *> res;
    for (AbstractResource *r : m_resources) {
        if (r->state() >= filter.state)
            res.push_back(r);
    }
    return new ResultsStream(QStringLiteral("OSTreeRPMStream"), res);
}

Transaction *RpmOstreeBackend::installApplication(AbstractResource *app, const AddonList &addons)
{
    updateCurrentDeployment();
    return new RpmOstreeTransaction(qobject_cast<RpmOstreeResource *>(app), addons, Transaction::InstallRole, transactionUpdatePath, true);
}

Transaction *RpmOstreeBackend::installApplication(AbstractResource *app)
{
    bool deploymentUpdate = isDeploymentUpdate;
    if (isDeploymentUpdate) {
        updateCurrentDeployment();
    } else {
        isDeploymentUpdate = true;
    }
    return new RpmOstreeTransaction(qobject_cast<RpmOstreeResource *>(app), Transaction::InstallRole, transactionUpdatePath, deploymentUpdate);
}

Transaction *RpmOstreeBackend::removeApplication(AbstractResource *)
{
    return nullptr;
}

void RpmOstreeBackend::updateCurrentDeployment()
{
    OrgProjectatomicRpmostree1OSInterface *m_interface = new OrgProjectatomicRpmostree1OSInterface(QStringLiteral("org.projectatomic.rpmostree1"),
                                                                                                   QStringLiteral("/org/projectatomic/rpmostree1/fedora"),
                                                                                                   QDBusConnection::systemBus(),
                                                                                                   this);
    QVariantMap options;
    QVariantMap modifiers;
    QString name;

    QDBusPendingReply<QString> reply = m_interface->UpdateDeployment(modifiers, options);
    reply.waitForFinished();
    if (!reply.isError()) {
        transactionUpdatePath = reply.argumentAt(0).value<QString>();
    } else {
        qWarning() << "Error occurs when performing the UpdateDeployment: " << reply.error();
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
    OrgProjectatomicRpmostree1OSInterface *m_interface = new OrgProjectatomicRpmostree1OSInterface(QStringLiteral("org.projectatomic.rpmostree1"),
                                                                                                   QStringLiteral("/org/projectatomic/rpmostree1/fedora"),
                                                                                                   QDBusConnection::systemBus(),
                                                                                                   this);
    isDeploymentUpdate = false;
    QVariantMap options;
    QStringList packages;

    QDBusPendingReply<QString> reply = m_interface->Rebase(options, selectedRefs, packages);
    reply.waitForFinished();
    if (!reply.isError()) {
        transactionUpdatePath = reply.argumentAt(0).value<QString>();
        installApplication(m_resources[0]);
    } else {
        qWarning() << "Error occurs when performing the Rebase: " << reply.error();
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
    return QStringLiteral("RpmOstree");
}

bool RpmOstreeBackend::hasApplications() const
{
    return true;
}

#include "RpmOstreeBackend.moc"
