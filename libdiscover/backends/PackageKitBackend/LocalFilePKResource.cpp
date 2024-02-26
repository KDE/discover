/*
 *   SPDX-FileCopyrightText: 2016 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "LocalFilePKResource.h"
#include "libdiscover_backend_debug.h"
#include <PackageKit/Daemon>
#include <PackageKit/Details>
#include <QDebug>
#include <QFileInfo>
#include <QProcess>
#include <utils.h>

LocalFilePKResource::LocalFilePKResource(QUrl path, PackageKitBackend *parent)
    : PackageKitResource(path.toString(), path.toString(), parent)
    , m_path(std::move(path))
{
}

quint64 LocalFilePKResource::size()
{
    const QFileInfo info(m_path.toLocalFile());
    return info.size();
}

QString LocalFilePKResource::name() const
{
    const QFileInfo info(m_path.toLocalFile());
    return info.baseName();
}

QString LocalFilePKResource::comment()
{
    return m_path.toLocalFile();
}

QString LocalFilePKResource::origin() const
{
    return m_path.toLocalFile();
}

void LocalFilePKResource::fetchDetails()
{
    if (!m_details.isEmpty()) {
        return;
    }
    m_details.insert(QStringLiteral("fetching"), true); // we add an entry so it's not re-fetched.

    if (PackageKit::Daemon::roles() & PackageKit::Transaction::RoleGetDetailsLocal) {
        PackageKit::Transaction *transaction = PackageKit::Daemon::getDetailsLocal(m_path.toLocalFile());
        connect(transaction, &PackageKit::Transaction::details, this, &LocalFilePKResource::resolve);
        connect(transaction, &PackageKit::Transaction::errorCode, this, &PackageKitResource::failedFetchingDetails);
    }

    if (PackageKit::Daemon::roles() & PackageKit::Transaction::RoleGetFilesLocal) {
        PackageKit::Transaction *transaction2 = PackageKit::Daemon::getFilesLocal(m_path.toLocalFile());
        connect(transaction2, &PackageKit::Transaction::errorCode, this, &PackageKitResource::failedFetchingDetails);
        connect(transaction2, &PackageKit::Transaction::files, this, [this](const QString & /*pkgid*/, const QStringList &files) {
            const auto execIdx = kIndexOf(files, [](const QString &file) {
                return file.endsWith(QLatin1String(".desktop")) && file.contains(QLatin1String("usr/share/applications"));
            });
            if (execIdx >= 0) {
                m_exec = files[execIdx];

                // sometimes aptcc provides paths like usr/share/applications/steam.desktop
                if (!m_exec.startsWith(QLatin1Char('/'))) {
                    m_exec.prepend(QLatin1Char('/'));
                }
            } else {
                qWarning() << "could not find an executable desktop file for" << m_path << "among" << files;
            }
        });
    }
}

void LocalFilePKResource::resolve(const PackageKit::Details &inDetails)
{
    clearPackageIds();
    const PackageKit::Details details = inDetails.isEmpty() ? m_details : inDetails;
    const auto detailsPackageId = details.packageId();
    auto resolve = PackageKit::Daemon::resolve(PackageKit::Daemon::packageName(detailsPackageId), PackageKit::Transaction::FilterNone);

    connect(resolve, &PackageKit::Transaction::package, this, [this, detailsPackageId](PackageKit::Transaction::Info info, const QString &packageId) {
        if (PackageKit::Daemon::packageName(packageId) == PackageKit::Daemon::packageName(detailsPackageId)
            && PackageKit::Daemon::packageVersion(packageId) == PackageKit::Daemon::packageVersion(detailsPackageId)
            && PackageKit::Daemon::packageArch(packageId) == PackageKit::Daemon::packageArch(detailsPackageId)
            && info == PackageKit::Transaction::InfoInstalled) {
            addPackageId(info, packageId, true);
        }
    });
    connect(resolve, &PackageKit::Transaction::finished, this, [this, details, detailsPackageId] {
        addPackageId(PackageKit::Transaction::InfoAvailable, detailsPackageId, true);
        PackageKitResource::setDetails(details);
    });
}

void LocalFilePKResource::invokeApplication() const
{
    KService::Ptr service = KService::Ptr(new KService(m_exec));

    if (!service) {
        return;
    }

    runService(service);
}

#include "moc_LocalFilePKResource.cpp"
