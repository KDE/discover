/*
 *   SPDX-FileCopyrightText: 2024 ivan tkachenko <me@ratijas.tk>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "PackageKitDependencies.h"
#include "PackageKitMessages.h"
#include "libdiscover_backend_debug.h"

#include <QDebug>

#include <PackageKit/Daemon>
#include <PackageKit/Transaction>

#include <variant>

using namespace Qt::StringLiterals;

PackageKitDependency::PackageKitDependency(PackageKit::Transaction::Info info, const QString &packageId, const QString &summary)
    : m_info(info)
    , m_infoString(PackageKitMessages::info(info))
    , m_packageId(packageId)
    , m_summary(summary)
{
}

bool PackageKitDependency::operator==(const PackageKitDependency &other) const
{
    // Skip derived fields.
    return m_info == other.m_info && m_packageId == other.m_packageId && m_summary == other.m_summary;
}

PackageKit::Transaction::Info PackageKitDependency::info() const
{
    return m_info;
}

QString PackageKitDependency::infoString() const
{
    return m_infoString;
}

QString PackageKitDependency::packageId() const
{
    return m_packageId;
}

QString PackageKitDependency::packageName() const
{
    return PackageKit::Transaction::packageName(m_packageId);
}

QString PackageKitDependency::summary() const
{
    return m_summary;
}

PackageKitDependencies::PackageKitDependencies(QObject *parent)
    : QObject(parent)
{
}

PackageKitDependencies::~PackageKitDependencies()
{
    cancel(false);
}

QString PackageKitDependencies::packageId() const
{
    return m_packageId;
}

void PackageKitDependencies::setPackageId(const QString &packageId)
{
    if (m_packageId != packageId) {
        m_packageId = packageId;
        cancel(true);
        Q_EMIT packageIdChanged();
    }
}

QList<PackageKitDependency> PackageKitDependencies::dependencies()
{
    if (m_state.has_value()) {
        if (auto list = std::get_if<Data>(&m_state.value())) {
            return *list;
        } else {
            // the job is still running
        }
    } else if (!m_packageId.isEmpty()) {
        // start the job
        Job job{new PakcageKitFetchDependenciesJob(m_packageId)};
        connect(job, &PakcageKitFetchDependenciesJob::finished, this, &PackageKitDependencies::onJobFinished);
        m_state = job;
    }
    return {};
}

void PackageKitDependencies::refresh()
{
    cancel(true);
    // force creation of a new job
    dependencies();
}

void PackageKitDependencies::onJobFinished(QList<PackageKitDependency> dependencies)
{
    Q_ASSERT(m_state.has_value());
    Q_ASSERT(std::holds_alternative<Job>(m_state.value()));

    if (auto job = std::get<Job>(m_state.value())) {
        // Should not emit twice, but better be on the safe side
        disconnect(job, &PakcageKitFetchDependenciesJob::finished, this, &PackageKitDependencies::onJobFinished);
    }

    m_state = dependencies;
    Q_EMIT dependenciesChanged();
}

void PackageKitDependencies::cancel(bool notify)
{
    if (m_state.has_value()) {
        if (auto jobPtr = std::get_if<Job>(&m_state.value())) {
            if (auto job = *jobPtr) {
                disconnect(job, &PakcageKitFetchDependenciesJob::finished, this, &PackageKitDependencies::onJobFinished);
                job->cancel();
            }
            notify = false;
        }
        m_state.reset();
        // first reset, only then notify and only if variant held Data before (i.e. not a Job)
        if (notify) {
            Q_EMIT dependenciesChanged();
        }
    }
}

PakcageKitFetchDependenciesJob::PakcageKitFetchDependenciesJob(const QString &packageId)
{
    if (packageId.isEmpty()) {
        onTransactionFinished();
        return;
    }

    m_transaction = PackageKit::Daemon::dependsOn(packageId);
    if (!m_transaction) {
        onTransactionFinished();
        return;
    }

    m_transaction->setParent(this);
    connect(m_transaction, &QObject::destroyed, this, &QObject::deleteLater);

    connect(m_transaction, &PackageKit::Transaction::errorCode, this, &PakcageKitFetchDependenciesJob::onTransactionErrorCode);
    connect(m_transaction, &PackageKit::Transaction::package, this, &PakcageKitFetchDependenciesJob::onTransactionPackage);
    connect(m_transaction, &PackageKit::Transaction::finished, this, &PakcageKitFetchDependenciesJob::onTransactionFinished);
}

PakcageKitFetchDependenciesJob::~PakcageKitFetchDependenciesJob()
{
    cancel();
}

void PakcageKitFetchDependenciesJob::cancel()
{
    if (m_transaction) {
        m_transaction->cancel();
    }
    deleteLater();
}

void PakcageKitFetchDependenciesJob::onTransactionErrorCode(PackageKit::Transaction::Error error, const QString &details)
{
    qCWarning(LIBDISCOVER_BACKEND_LOG) << "PakcageKitFetchDependenciesJob: Transaction error:" << m_transaction << error << details;
}

void PakcageKitFetchDependenciesJob::onTransactionPackage(PackageKit::Transaction::Info info, const QString &packageId, const QString &summary)
{
    m_dependencies.append(PackageKitDependency(info, packageId, summary));
}

void PakcageKitFetchDependenciesJob::onTransactionFinished()
{
    std::sort(m_dependencies.begin(), m_dependencies.end(), [](const PackageKitDependency &a, const PackageKitDependency &b) {
        return a.info() < b.info() || (a.info() == b.info() && a.packageName() < b.packageName());
    });

    Q_EMIT finished(m_dependencies);

    deleteLater();
}

#include "moc_PackageKitDependencies.cpp"
