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

#include <ostree-repo.h>
#include <ostree.h>

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
    filterRemoteRefs();
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

QStringList RpmOstreeBackend::getRemoteRefs(const QString &remote)
{
    QStringList r;

    g_autoptr(GFile) path = g_file_new_for_path("/ostree/repo");
    g_autoptr(OstreeRepo) repo = ostree_repo_new(path);
    if (repo == NULL) {
        qWarning() << "rpm-ostree-backend: Could not find ostree repo:" << path;
        return r;
    }

    g_autoptr(GError) err = NULL;
    gboolean res = ostree_repo_open(repo, NULL, &err);
    if (!res) {
        qWarning() << "rpm-ostree-backend: Could not open ostree repo:" << path;
        return r;
    }

    g_autoptr(GHashTable) refs;
    QByteArray rem = remote.toLocal8Bit();
    res = ostree_repo_remote_list_refs(repo, rem.data(), &refs, NULL, &err);
    if (!res) {
        qWarning() << "rpm-ostree-backend: Could not get the list of refs for ostree repo:" << path;
        return r;
    }

    GHashTableIter iter;
    gpointer key, value;
    g_hash_table_iter_init(&iter, refs);
    while (g_hash_table_iter_next(&iter, &key, &value)) {
        r << QString((char *)key);
    }

    if (r.size() == 0) {
        qWarning() << "rpm-ostree-backend: Could not find any remote red for ostree repo:" << path;
    }

    return r;
}

void RpmOstreeBackend::filterRemoteRefs()
{
    auto currentRemote = m_resources[0]->getRemote();
    auto refs = getRemoteRefs(currentRemote);

    QStringList remoteRefs;
    auto name = m_resources[0]->getBranchName();
    auto version = m_resources[0]->getBranchVersion();
    auto arch = m_resources[0]->getBranchArch();
    auto variant = m_resources[0]->getBranchVariant();

    for (int i = 0; i < refs.size(); ++i) {
        // Split branch into name / version / arch / variant
        auto split_branch = refs.at(i).split('/');
        if (split_branch.length() < 4) {
            qWarning() << "rpm-ostree-backend: Unknown branch format, ignoring:" << refs.at(i);
            continue;
        } else {
            auto refVariant = split_branch[3];
            for (int i = 4; i < split_branch.size(); ++i) {
                refVariant += "/" + split_branch[i];
            }
            if (split_branch[0] == name && split_branch[2] == arch && refVariant == variant) {
                remoteRefs.push_back(refs.at(i));
            }
        }
    }
    if (remoteRefs.size() == 0) {
        qWarning() << "rpm-ostree-backend: Found no matching branch in remote:" << currentRemote;
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
        qWarning() << "rpm-ostree-backend: Error occurs when performing the UpdateDeployment: " << reply.error();
    }
}

void RpmOstreeBackend::checkForUpdates()
{
    if (m_fetching)
        return;
    executeCheckUpdateProcess();
    filterRemoteRefs();
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
