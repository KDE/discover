/*
 *   SPDX-FileCopyrightText: 2025 Lasath Fernando <devel@lasath.org>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "SystemdSysupdateTransaction.h"
#include "SystemdSysupdateBackend.h"
#include "libdiscover_systemdsysupdate_debug.h"

#include <QCoroDBusPendingReply>

#define SYSTEMDSYSUPDATE_LOG LIBDISCOVER_BACKEND_SYSTEMDSYSUPDATE_LOG

constexpr QLatin1StringView PROGRESS_PROPERTY_NAME("Progress");
const QLatin1StringView SYSUPDATE_JOB_INTERFACE_NAME = QLatin1String(org::freedesktop::sysupdate1::Job::staticInterfaceName());

SystemdSysupdateTransaction::SystemdSysupdateTransaction(AbstractResource *resource, SystemdSysupdateUpdateReply &updateCall)
    : Transaction(resource, resource, InstallRole)
{
    // Can't cancel until we have a job ID
    setCancellable(false);
    setStatus(DownloadingStatus);

    auto watcher = new QDBusPendingCallWatcher(updateCall, this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, [this, resource](QDBusPendingCallWatcher *watcher) {
        watcher->deleteLater();
        const SystemdSysupdateUpdateReply reply = *watcher;

        if (reply.isError()) {
            qCCritical(SYSTEMDSYSUPDATE_LOG) << "Failed to create job:" << reply.error().message();
            Q_EMIT passiveMessage(reply.error().message());
            setStatus(DoneWithErrorStatus);
            return;
        }

        auto id = reply.argumentAt<1>();
        auto path = reply.argumentAt<2>();

        qCDebug(SYSTEMDSYSUPDATE_LOG) << "Created sysupdate1::Job with path " << path;

        // need to keep a reference to this to be able to cancel
        m_job = new org::freedesktop::sysupdate1::Job(SYSUPDATE1_SERVICE, path.path(), SystemdSysupdateBackend::OUR_BUS(), this);
        m_job->setInteractiveAuthorizationAllowed(true); // cancel may require authorization
        setCancellable(true);

        // Don't need to keep this around because we're just connecting to some signals (and it'll be deleted by Qt with the parent)
        auto *properties = new org::freedesktop::DBus::Properties(SYSUPDATE1_SERVICE, path.path(), SystemdSysupdateBackend::OUR_BUS(), this);
        connect(properties,
                &org::freedesktop::DBus::Properties::PropertiesChanged,
                this,
                [this, properties](const QString &interface, const QVariantMap &changed, const QStringList &invalidated) -> QCoro::Task<> {
                    if (interface != SYSUPDATE_JOB_INTERFACE_NAME) {
                        co_return;
                    }

                    qCDebug(SYSTEMDSYSUPDATE_LOG) << "Properties changed:" << changed << "Invalidated:" << invalidated;

                    if (changed.contains(PROGRESS_PROPERTY_NAME)) {
                        setProgress(changed.value(PROGRESS_PROPERTY_NAME).toUInt());
                    }

                    if (invalidated.contains(PROGRESS_PROPERTY_NAME)) {
                        const auto reply = co_await properties->Get(SYSUPDATE_JOB_INTERFACE_NAME, PROGRESS_PROPERTY_NAME);
                        if (reply.isError()) {
                            qCCritical(SYSTEMDSYSUPDATE_LOG) << "Failed to get progress:" << reply.error().message();
                            co_return;
                        }

                        setProgress(reply.argumentAt(0).toUInt());
                    }
                });

        auto backend = qobject_cast<SystemdSysupdateBackend *>(resource->backend());
        Q_ASSERT(backend);
        connect(backend,
                &SystemdSysupdateBackend::transactionRemoved,
                this,
                [id, resource, this](qulonglong jobId, const QDBusObjectPath &jobPath, int status) {
                    if (id != jobId) {
                        return;
                    }

                    qCInfo(SYSTEMDSYSUPDATE_LOG) << "Job" << jobPath.path() << "for target" << resource->name() << "finished with status" << status;
                    setStatus(status == 0 ? DoneStatus : DoneWithErrorStatus);
                    deleteLater();
                });
    });
}

void SystemdSysupdateTransaction::cancel()
{
    if (!m_job) {
        qWarning(SYSTEMDSYSUPDATE_LOG) << "Can't cancel transaction without a job";
        return;
    }

    auto watcher = new QDBusPendingCallWatcher(m_job->Cancel(), this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, [](QDBusPendingCallWatcher *watcher) {
        watcher->deleteLater();
        if (watcher->isError()) {
            qWarning(SYSTEMDSYSUPDATE_LOG) << "Failed to cancel job:" << watcher->error().message();
            return;
        }

        qDebug(SYSTEMDSYSUPDATE_LOG) << "Job cancelled";
    });
    setStatus(CancelledStatus);
}
