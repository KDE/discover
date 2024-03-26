/*
 *   SPDX-FileCopyrightText: 2024 ivan tkachenko <me@ratijas.tk>
 *   SPDX-FileCopyrightText: 2024 Harald Sitter <sitter@kde.org>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "PackageKitDependencies.h"
#include "PackageKitMessages.h"
#include "libdiscover_backend_packagekit_debug.h"

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

bool PackageKitDependencies::hasFetchedDependencies()
{
    return m_state.has_value() && std::holds_alternative<Data>(*m_state);
}

QList<PackageKitDependency> PackageKitDependencies::dependencies()
{
    Q_ASSERT(m_state.has_value()); // start must have been called before!
    if (auto list = std::get_if<Data>(&m_state.value())) {
        return *list;
    }
    // the job is still running
    return {};
}

void PackageKitDependencies::start()
{
    Q_ASSERT(!m_state.has_value()); // cancel must have been called before!
    Job job{new PackageKitFetchDependenciesJob(m_packageId)};
    connect(job, &PackageKitFetchDependenciesJob::finished, this, &PackageKitDependencies::onJobFinished);
    m_state = job;
}

void PackageKitDependencies::refresh()
{
    cancel(true);
    start();
}

void PackageKitDependencies::onJobFinished(QList<PackageKitDependency> dependencies)
{
    Q_ASSERT(m_state.has_value());
    Q_ASSERT(std::holds_alternative<Job>(m_state.value()));

    if (auto job = std::get<Job>(m_state.value())) {
        // Should not emit twice, but better be on the safe side
        disconnect(job, &PackageKitFetchDependenciesJob::finished, this, &PackageKitDependencies::onJobFinished);
    }

    m_state = dependencies;
    Q_EMIT dependenciesChanged();
}

void PackageKitDependencies::cancel(bool notify)
{
    if (m_state.has_value()) {
        if (auto jobPtr = std::get_if<Job>(&m_state.value())) {
            if (auto job = *jobPtr) {
                disconnect(job, &PackageKitFetchDependenciesJob::finished, this, &PackageKitDependencies::onJobFinished);
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

PackageKitFetchDependenciesJob::PackageKitFetchDependenciesJob(const QString &packageId)
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

    connect(m_transaction, &PackageKit::Transaction::errorCode, this, &PackageKitFetchDependenciesJob::onTransactionErrorCode);
    connect(m_transaction, &PackageKit::Transaction::package, this, &PackageKitFetchDependenciesJob::onTransactionPackage);
    connect(m_transaction, &PackageKit::Transaction::finished, this, &PackageKitFetchDependenciesJob::onTransactionFinished);
}

PackageKitFetchDependenciesJob::~PackageKitFetchDependenciesJob()
{
    cancel();
}

void PackageKitFetchDependenciesJob::cancel()
{
    if (m_transaction) {
        m_transaction->cancel();
    }
    deleteLater();
}

void PackageKitFetchDependenciesJob::onTransactionErrorCode(PackageKit::Transaction::Error error, const QString &details)
{
    qCWarning(LIBDISCOVER_BACKEND_PACKAGEKIT_LOG) << "PackageKitFetchDependenciesJob: Transaction error:" << m_transaction << error << details;
}

void PackageKitFetchDependenciesJob::onTransactionPackage(PackageKit::Transaction::Info info, const QString &packageId, const QString &summary)
{
    m_dependencies.append(PackageKitDependency(info, packageId, summary));
}

void PackageKitFetchDependenciesJob::onTransactionFinished()
{
    std::sort(m_dependencies.begin(), m_dependencies.end(), [](const PackageKitDependency &a, const PackageKitDependency &b) {
        return a.info() < b.info() || (a.info() == b.info() && a.packageName() < b.packageName());
    });

    Q_EMIT finished(m_dependencies);

    deleteLater();
}

void PackageKitDependencies::setDirty()
{
    cancel(true);
}

#include "moc_PackageKitDependencies.cpp"
