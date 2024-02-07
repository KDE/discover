/*
 *   SPDX-FileCopyrightText: 2022 Jeremy Whiting <jeremy.whiting@collabora.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "SteamOSBackend.h"
#include "SteamOSResource.h"
#include "SteamOSTransaction.h"
#include <Transaction/Transaction.h>
#include <resources/SourcesModel.h>
#include <resources/StandardBackendUpdater.h>

#include <KAboutData>
#include <KConfigGroup>
#include <KLocalizedString>
#include <KPluginFactory>
#include <KSharedConfig>

#include <QDBusArgument>
#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusPendingCall>
#include <QDBusPendingCallWatcher>
#include <QDebug>
#include <QFile>
#include <QThread>
#include <QTimer>

#include <QCoro/QCoroDBus>

#include "atomupd1.h"
#include "libdiscover_steamos_debug.h"

DISCOVER_BACKEND_PLUGIN(SteamOSBackend)

// We expect 2 results, updates and later updates
#define CHECK_UPDATES_RETURN_COUNT 2
#define ATOMUPD_SERVICE_PATH "/usr/lib/systemd/system/atomupd.service"

QString SteamOSBackend::service()
{
    return QStringLiteral("com.steampowered.Atomupd1");
}

QString SteamOSBackend::path()
{
    return QStringLiteral("/com/steampowered/Atomupd1");
}

SteamOSBackend::SteamOSBackend(QObject *parent)
    : AbstractResourcesBackend(parent)
    , m_updater(new StandardBackendUpdater(this))
    , m_updateVersion()
    , m_updateSize(0)
    , m_resource(nullptr)
{
    qDBusRegisterMetaType<VariantMapMap>();

    connect(m_updater, &StandardBackendUpdater::updatesCountChanged, this, &SteamOSBackend::updatesCountChanged);

    m_interface = new ComSteampoweredAtomupd1Interface(service(), path(), QDBusConnection::systemBus(), this);

    // First try to get the current version, only check valid after that since
    // this could wake up the service
    m_currentVersion = m_interface->currentVersion();
    m_currentBuildID = m_interface->currentBuildID();
    qCDebug(LIBDISCOVER_BACKEND_STEAMOS_LOG) << "steamos-backend: Current version from dbus api: " << m_currentVersion << " and build ID: " << m_currentBuildID;

    // If we got a version property, assume the service is responding and check for updates
    if (!m_currentVersion.isEmpty() && !m_currentBuildID.isEmpty()) {
        checkForUpdates();
    } else {
        qCDebug(LIBDISCOVER_BACKEND_STEAMOS_LOG) << "steamos-backend: Unable to query atomupd for SteamOS Updates...";
        // Should never happen, since trying to open the interface above starts
        // it, but if this plugin is on non steamos devices we should show something
        Q_EMIT passiveMessage(i18n("SteamOS: Unable to query atomupd for SteamOS Updates..."));
    }
}

void SteamOSBackend::hasUpdateChanged(bool hasUpdate)
{
    if (hasUpdate) {
        // Create or update resource from m_updateVersion, m_updateBuild
        if (!m_resource) {
            qCDebug(LIBDISCOVER_BACKEND_STEAMOS_LOG) << "steamos-backend: Creating new SteamOSResource with build id: " << m_updateBuild;
            m_resource =
                new SteamOSResource(m_updateVersion, m_updateBuild, m_updateSize, QStringLiteral("%1 - %2").arg(m_currentVersion).arg(m_currentBuildID), this);
        } else {
            qCDebug(LIBDISCOVER_BACKEND_STEAMOS_LOG) << "steamos-backend: Updating SteamOSResource with new version: " << m_updateVersion
                                                     << " and new build id: " << m_updateBuild;
            m_resource->setVersion(m_updateVersion);
            m_resource->setBuild(m_updateBuild);
            Q_EMIT m_resource->versionsChanged();
        }
    } else {
        // Clear or remove any previously created resource
    }

    qCDebug(LIBDISCOVER_BACKEND_STEAMOS_LOG) << "steamos-backend: Updates count is now " << updatesCount();
}

void SteamOSBackend::needRebootChanged()
{
    // Tell gui we need to reboot
    m_updater->setNeedsReboot(true);
}

void SteamOSBackend::acquireFetching(bool f)
{
    if (f) {
        m_fetching++;
    } else {
        m_fetching--;
    }

    if ((!f && m_fetching == 0) || (f && m_fetching == 1)) {
        Q_EMIT fetchingChanged();
    }
}

void SteamOSBackend::checkForUpdates()
{
    // Can't turn an overridden method into a coroutine
    checkForUpdatesTask();
}

QCoro::Task<> SteamOSBackend::checkForUpdatesTask()
{
    QPointer<SteamOSBackend> guard = this;

    if (m_fetching) {
        co_return;
    }

    // If there's no dbus object to talk to, just return
    // TODO: Maybe show a warning/error?
    if (m_currentVersion.isEmpty()) {
        co_return;
    }

    acquireFetching(true);

    qCDebug(LIBDISCOVER_BACKEND_STEAMOS_LOG) << "steamos-backend-backend::checkForUpdates asking DBus api";
    // We don't send any options for now, dbus api doesn't do anything with them yet anyway
    QDBusPendingReply<VariantMapMap, VariantMapMap> reply = co_await m_interface->CheckForUpdates({});

    if (!guard) {
        co_return;
    }

    if (reply.isError()) {
        Q_EMIT passiveMessage(reply.error().message());
        qCDebug(LIBDISCOVER_BACKEND_STEAMOS_LOG) << "steamos-backend: CheckForUpdates error: " << reply.error().message();
    } else {
        // Valid response, parse it
        VariantMapMap versions = reply.argumentAt<0>();
        qCDebug(LIBDISCOVER_BACKEND_STEAMOS_LOG) << "steamos-backend-backend: Versions available: " << versions;
        VariantMapMap laterVersions = reply.argumentAt<1>();

        if (versions.isEmpty()) {
            // No updates
            hasUpdateChanged(false);
        } else {
            m_updateBuild = versions.keys().at(0);
            QVariantMap data = versions.value(m_updateBuild);
            m_updateVersion = data.value(QLatin1String("version")).toString();
            m_updateSize = data.value(QLatin1String("estimated_size")).toUInt();
            qCDebug(LIBDISCOVER_BACKEND_STEAMOS_LOG) << "steamos-backend: Data values: " << data.values();
            hasUpdateChanged(true);
        }
    }
    acquireFetching(false);
}

int SteamOSBackend::updatesCount() const
{
    return m_updater->updatesCount();
}

// SteamOS never has any searchable packages or anything, so just always
// give a void stream
ResultsStream *SteamOSBackend::search(const AbstractResourcesBackend::Filters &filter)
{
    QVector<StreamResult> res;
    if (m_resource && m_resource->state() >= filter.state) {
        res << StreamResult{m_resource, 0};
    }
    return new ResultsStream(QLatin1String("SteamOS-stream"), res);
}

QHash<QString, SteamOSResource *> SteamOSBackend::resources() const
{
    return m_resources;
}

bool SteamOSBackend::isValid() const
{
    return QFile(QStringLiteral(ATOMUPD_SERVICE_PATH)).exists();
}

AbstractBackendUpdater *SteamOSBackend::backendUpdater() const
{
    return m_updater;
}

AbstractReviewsBackend *SteamOSBackend::reviewsBackend() const
{
    return nullptr;
}

Transaction *SteamOSBackend::installApplication(AbstractResource *app, const AddonList &addons)
{
    Q_UNUSED(addons);
    return installApplication(app);
}

Transaction *SteamOSBackend::installApplication(AbstractResource *app)
{
    SteamOSTransaction *transaction = new SteamOSTransaction(qobject_cast<SteamOSResource *>(app), Transaction::InstallRole, m_interface);
    connect(transaction, &SteamOSTransaction::needReboot, this, &SteamOSBackend::needRebootChanged);
    return transaction;
}

Transaction *SteamOSBackend::removeApplication(AbstractResource *)
{
    qWarning() << "steamos-backend: Unsupported operation:" << __PRETTY_FUNCTION__;
    return nullptr;
}

bool SteamOSBackend::isFetching() const
{
    return m_fetching > 0;
}

QString SteamOSBackend::displayName() const
{
    return QStringLiteral("SteamOS");
}

#include "SteamOSBackend.moc"
#include "moc_SteamOSBackend.cpp"
