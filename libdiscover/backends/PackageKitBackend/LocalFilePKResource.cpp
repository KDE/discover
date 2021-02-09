/*
 *   SPDX-FileCopyrightText: 2016 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "LocalFilePKResource.h"
#include <QDebug>
#include <QFileInfo>
#include <QProcess>
#include <PackageKit/Daemon>
#include <PackageKit/Details>
#include <utils.h>
#include "config-paths.h"
#include "libdiscover_backend_debug.h"

LocalFilePKResource::LocalFilePKResource(QUrl path, PackageKitBackend* parent)
    : PackageKitResource(path.toString(), path.toString(), parent)
    , m_path(std::move(path))
{
}

int LocalFilePKResource::size()
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

void LocalFilePKResource::markInstalled()
{
    m_state = AbstractResource::Installed;
    Q_EMIT stateChanged();
}

QString LocalFilePKResource::origin() const
{
    return m_path.toLocalFile();
}

void LocalFilePKResource::fetchDetails()
{
    if (!m_details.isEmpty())
        return;
    m_details.insert(QStringLiteral("fetching"), true);//we add an entry so it's not re-fetched.

    PackageKit::Transaction* transaction = PackageKit::Daemon::getDetailsLocal(m_path.toLocalFile());
    connect(transaction, &PackageKit::Transaction::details, this, &LocalFilePKResource::setDetails);
    connect(transaction, &PackageKit::Transaction::errorCode, this, &PackageKitResource::failedFetchingDetails);

    PackageKit::Transaction* transaction2 = PackageKit::Daemon::getFilesLocal(m_path.toLocalFile());
    connect(transaction2, &PackageKit::Transaction::errorCode, this, &PackageKitResource::failedFetchingDetails);
    connect(transaction2, &PackageKit::Transaction::files, this, [this] (const QString &/*pkgid*/, const QStringList & files) {
        const auto execIdx = kIndexOf(files, [](const QString& file) { return file.endsWith(QLatin1String(".desktop")) && file.contains(QLatin1String("usr/share/applications")); });
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
    connect(transaction2, &PackageKit::Transaction::finished, this, [] {qCDebug(LIBDISCOVER_BACKEND_LOG) << "."; });
}

void LocalFilePKResource::setDetails(const PackageKit::Details& details)
{
    addPackageId(PackageKit::Transaction::InfoAvailable, details.packageId(), true);
    PackageKitResource::setDetails(details);
}

void LocalFilePKResource::invokeApplication() const
{
    QProcess::startDetached(QStringLiteral(CMAKE_INSTALL_FULL_LIBEXECDIR_KF5 "/discover/runservice"), {m_exec});
}
