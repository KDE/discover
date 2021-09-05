/*
 *   SPDX-FileCopyrightText: 2020 Aleix Pol Gonzalez <aleixpol@kde.org>
 *   SPDX-FileCopyrightText: 2021 Mariam Fahmy Sobhy <mariamfahmy66@gmail.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "RpmOstreeBackend.h"
#include "RpmOstreeResource.h"
#include "RpmOstreeSourcesBackend.h"
#include "RpmOstreeTransaction.h"

#include <Transaction/Transaction.h>
#include <resources/StandardBackendUpdater.h>

#include <QDBusInterface>
#include <QDBusMessage>
#include <QDBusObjectPath>
#include <QDBusPendingReply>
#include <QDebug>
#include <QFile>
#include <QList>
#include <QMap>
#include <QProcess>
#include <QStandardItemModel>
#include <QStringList>
#include <QVariant>
#include <QVariantList>

DISCOVER_BACKEND_PLUGIN(RpmOstreeBackend)

RpmOstreeBackend::RpmOstreeBackend(QObject *parent)
    : AbstractResourcesBackend(parent)
    , m_updater(new StandardBackendUpdater(this))
    , m_fetching(false)
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
        // Only include the currently booted deployment for now
        if (map[QStringLiteral("booted")].toBool()) {
            RpmOstreeResource *deploymentResource = new RpmOstreeResource(map, this);
            // changing the state of the booted deployment resource to Installed.
            deploymentResource->setState(AbstractResource::Installed);
            connect(deploymentResource, &RpmOstreeResource::stateChanged, this, &RpmOstreeBackend::updatesCountChanged);
            m_resources.push_back(deploymentResource);
        }
    }
    dbusArgs.endArray();
}

void RpmOstreeBackend::toggleFetching()
{
    m_fetching = !m_fetching;
    Q_EMIT fetchingChanged();
}

void RpmOstreeBackend::executeCheckUpdateProcess()
{
    toggleFetching();
    QProcess *process = new QProcess(this);

    connect(process, &QProcess::readyReadStandardError, [process]() {
        qWarning() << "rpm-ostree-backend: Error while calling rpm-ostree:" << process->readAllStandardError();
    });
    connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), [this, process](int exitCode, QProcess::ExitStatus exitStatus) {
        if (exitStatus != QProcess::NormalExit) {
            qWarning() << "rpm-ostree-backend: Error while calling rpm-ostree:" << process->readAllStandardError();
            toggleFetching();
            return;
        }
        if (exitCode != 0) {
            qInfo() << "rpm-ostree-backend: No update available";
            toggleFetching();
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
            newVersionFound.remove(0, 25);
            newVersionFound.remove(13, newVersionFound.size() - 13);
            m_resources[0]->setNewVersion(newVersionFound);
            m_resources[0]->setState(AbstractResource::Upgradeable);
        }
        toggleFetching();
        process->deleteLater();
    });
    process->setProcessChannelMode(QProcess::MergedChannels);
    auto prog = QStringLiteral("rpm-ostree");
    auto args = {QStringLiteral("update"), QStringLiteral("--check")};
    process->start(prog, args);
}

void RpmOstreeBackend::executeRemoteRefsProcess()
{
    QProcess *process = new QProcess(this);

    connect(process, &QProcess::readyReadStandardError, [process]() {
        qWarning() << "rpm-ostree-backend: Error while calling rpm-ostree:" << process->readAllStandardError().constData();
    });
    QObject::connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), [=](int exitCode, QProcess::ExitStatus exitStatus) {
        if (exitStatus != QProcess::NormalExit) {
            qWarning() << "rpm-ostree-backend: Error while calling ostree:" << process->readAllStandardError();
            return;
        }
        if (exitCode != 0) {
            qWarning() << "rpm-ostree-backend: Error while calling ostree:" << process->readAllStandardError();
            return;
        }
        const QString kinoite = QStringLiteral("/kinoite");

        QStringList remoteRefs;
        QTextStream stream(process);
        for (QString ref = stream.readLine(); stream.readLineInto(&ref);) {
            if (ref.endsWith(kinoite))
                continue;
            remoteRefs.push_back(ref);
        }
        m_resources[0]->setRemoteRefsList(remoteRefs);
        process->deleteLater();
    });
    process->setProcessChannelMode(QProcess::MergedChannels);
    auto prog = QStringLiteral("ostree");
    auto args = {QStringLiteral("--repo=/ostree/repo"), QStringLiteral("remote"), QStringLiteral("refs"), QStringLiteral("fedora")};
    process->start(prog, args);
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
        qWarning() << "rpm-ostree-backend: Error occurs when performing the UpdateDeployment: " << reply.error();
    }
}

void RpmOstreeBackend::checkForUpdates()
{
    if (m_fetching)
        return;
    executeCheckUpdateProcess();
    executeRemoteRefsProcess();
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
        qWarning() << "rpm-ostree-backend: Error occurs when performing the Rebase: " << reply.error();
    }
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

#include "RpmOstreeBackend.moc"
