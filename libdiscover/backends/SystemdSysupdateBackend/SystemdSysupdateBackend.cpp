/*
 *   SPDX-FileCopyrightText: 2024 Lasath Fernando <devel@lasath.org>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "SystemdSysupdateBackend.h"
#include "SystemdSysupdateResource.h"
#include "SystemdSysupdateTransaction.h"
#include "libdiscover_systemdsysupdate_debug.h"

#include <AppStreamQt/metadata.h>
#include <QCoro/QCoroDBusPendingReply>
#include <QCoro/QCoroNetworkReply>
#include <QList>
#include <QNetworkReply>
#include <QtPreprocessorSupport>
#include <resources/AbstractResource.h>
#include <resources/AbstractResourcesBackend.h>

DISCOVER_BACKEND_PLUGIN(SystemdSysupdateBackend)

#define SYSTEMDSYSUPDATE_LOG LIBDISCOVER_BACKEND_SYSTEMDSYSUPDATE_LOG

const auto path = QStringLiteral("/org/freedesktop/sysupdate1");

SystemdSysupdateBackend::SystemdSysupdateBackend(QObject *parent)
    : AbstractResourcesBackend(parent)
    , m_updater(new StandardBackendUpdater(this))
    , m_manager(new org::freedesktop::sysupdate1::Manager(SYSUPDATE1_SERVICE, path, QDBusConnection::systemBus(), this))
    , m_nam(new QNetworkAccessManager(this))
{
    qDBusRegisterMetaType<Sysupdate::Target>();
    qDBusRegisterMetaType<Sysupdate::TargetList>();

    connect(m_manager, &org::freedesktop::sysupdate1::Manager::JobRemoved, this, &SystemdSysupdateBackend::transactionRemoved);
    connect(m_updater, &StandardBackendUpdater::updatesCountChanged, this, &SystemdSysupdateBackend::updatesCountChanged);
    QTimer::singleShot(0, this, &SystemdSysupdateBackend::checkForUpdates);
}

int SystemdSysupdateBackend::updatesCount() const
{
    return m_updater->updatesCount();
}

bool SystemdSysupdateBackend::isValid() const
{
    auto ping = org::freedesktop::DBus::Peer(SYSUPDATE1_SERVICE, path, QDBusConnection::systemBus()).Ping();
    ping.waitForFinished();
    return !ping.isError();
}

ResultsStream *SystemdSysupdateBackend::search(const AbstractResourcesBackend::Filters &filter)
{
    Q_UNUSED(filter);

    // Since we'll only ever have a handful of targets, we can just return all of them
    QVector<StreamResult> results;
    for (const auto &resource : m_resources) {
        results << StreamResult(resource);
    }

    return new ResultsStream(QStringLiteral("systemd-sysupdate"), results);
}

AbstractBackendUpdater *SystemdSysupdateBackend::backendUpdater() const
{
    return m_updater;
}

AbstractReviewsBackend *SystemdSysupdateBackend::reviewsBackend() const
{
    return nullptr;
}

Transaction *SystemdSysupdateBackend::installApplication(AbstractResource *app)
{
    const auto resource = qobject_cast<SystemdSysupdateResource *>(app);
    if (!resource) {
        qCCritical(SYSTEMDSYSUPDATE_LOG) << "Failed to cast resource to SystemdSysupdateResource";
        return nullptr;
    }

    return resource->update();
}

Transaction *SystemdSysupdateBackend::installApplication(AbstractResource *app, const AddonList &addons)
{
    Q_UNUSED(addons);
    return installApplication(app);
}

Transaction *SystemdSysupdateBackend::removeApplication(AbstractResource *app)
{
    Q_UNUSED(app);
    return nullptr;
}

bool SystemdSysupdateBackend::isFetching() const
{
    return m_fetchOperationCount > 0;
}

void SystemdSysupdateBackend::checkForUpdates()
{
    qCDebug(SYSTEMDSYSUPDATE_LOG) << "Updating systemd-sysupdate backend...";
    checkForUpdatesAsync();
}

QCoro::Task<> SystemdSysupdateBackend::checkForUpdatesAsync()
{
    if (isFetching()) {
        qCInfo(SYSTEMDSYSUPDATE_LOG) << "Already fetching updates. Skipping...";
        co_return;
    }

    beginFetch();

    for (const auto &resource : m_resources) {
        Q_EMIT resourceRemoved(resource);
    }
    m_resources.clear();

    const auto targetsReply = co_await m_manager->ListTargets();
    if (targetsReply.isError()) {
        qCWarning(SYSTEMDSYSUPDATE_LOG) << "Failed to list targets:" << targetsReply.error().message();
        co_return;
    }

    // TODO: Make this parallel once QCoro2 is released
    // https://github.com/qcoro/qcoro/issues/250
    for (const auto &[targetClass, name, objectPath] : targetsReply.value()) {
        qCDebug(SYSTEMDSYSUPDATE_LOG) << "Target:" << name << targetClass << objectPath.path();

        auto target = new org::freedesktop::sysupdate1::Target(SYSUPDATE1_SERVICE, objectPath.path(), QDBusConnection::systemBus(), this);
        target->setInteractiveAuthorizationAllowed(true); // in case Update() needs authentication
        const auto appStream = co_await target->GetAppStream();
        if (appStream.isError()) {
            qCCritical(SYSTEMDSYSUPDATE_LOG) << "Failed to get appstream for target (" << name << ") :" << appStream.error().message();
            continue;
        }
        auto appStreamUrls = appStream.value();
        if (appStreamUrls.isEmpty()) {
            qCritical(SYSTEMDSYSUPDATE_LOG) << "No appstream URLs found for target:" << name;
            continue;
        }

        qCDebug(SYSTEMDSYSUPDATE_LOG) << "AppStream:" << appStreamUrls;
        AppStream::Metadata metadata;
        for (const auto &url : appStreamUrls) {
            const auto reply = co_await m_nam->get(QNetworkRequest(QUrl(url)));
            if (reply->error() != QNetworkReply::NoError) {
                qCWarning(SYSTEMDSYSUPDATE_LOG) << "Failed to fetch appstream:" << reply->errorString();
                continue;
            }

            // if the first non-whitespace character is not a '<', it's probably a YAML file
            auto data = QString::fromUtf8(reply->readAll()).trimmed();
            auto format = data.startsWith(QLatin1Char('<')) ? AppStream::Metadata::FormatKindXml : AppStream::Metadata::FormatKindYaml;

            auto error = metadata.parse(data, format);
            if (error != AppStream::Metadata::MetadataErrorNoError) {
                qCCritical(SYSTEMDSYSUPDATE_LOG) << "Failed to parse appstream metadata for target:" << name << ": " << error << " - " << metadata.lastError();
                continue;
            } else {
                qCDebug(SYSTEMDSYSUPDATE_LOG) << "Successfully parsed appstream metadata for target:" << name;
            }
        }

        auto components = metadata.components();
        if (components.isEmpty()) {
            qCCritical(SYSTEMDSYSUPDATE_LOG) << "No components found in appstream metadata for target:" << name;
            continue;
        }

        if (components.size() > 1) {
            qCWarning(SYSTEMDSYSUPDATE_LOG) << "Multiple components found in appstream metadata for target:" << name << ". Using the first one.";
        }

        auto component = metadata.component();
        qCDebug(SYSTEMDSYSUPDATE_LOG) << "Component:" << component.name() << component.summary() << component.description();

        QString installedVersion = co_await target->GetVersion();
        QString availableVersion = co_await target->CheckNew();
        qCDebug(SYSTEMDSYSUPDATE_LOG) << "Installed version:" << installedVersion << "Available version:" << availableVersion;

        if (installedVersion.isEmpty()) {
            qCWarning(SYSTEMDSYSUPDATE_LOG) << "Failed to get installed version for target:" << name;
            continue;
        }

        if (availableVersion.isEmpty()) {
            qCInfo(SYSTEMDSYSUPDATE_LOG) << "No new version available for target:" << name;
            continue;
        }

        m_resources << new SystemdSysupdateResource(this, component, {installedVersion, availableVersion}, target);
    }

    endFetch();
}

QString SystemdSysupdateBackend::displayName() const
{
    return QStringLiteral("Systemd SysUpdate");
}

void SystemdSysupdateBackend::beginFetch()
{
    m_fetchOperationCount++;
    if (m_fetchOperationCount == 1) {
        Q_EMIT fetchingChanged();
    }
}
void SystemdSysupdateBackend::endFetch()
{
    m_fetchOperationCount--;
    if (m_fetchOperationCount == 0) {
        Q_EMIT fetchingChanged();
    }
}

#include "SystemdSysupdateBackend.moc"