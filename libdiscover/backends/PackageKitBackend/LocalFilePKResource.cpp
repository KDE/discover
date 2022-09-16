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
    if (!m_details.isEmpty())
        return;
    m_details.insert(QStringLiteral("fetching"), true); // we add an entry so it's not re-fetched.

    if (PackageKit::Daemon::roles() & PackageKit::Transaction::RoleGetDetailsLocal) {
        PackageKit::Transaction *transaction = PackageKit::Daemon::getDetailsLocal(m_path.toLocalFile());
        connect(transaction, &PackageKit::Transaction::details, this, &LocalFilePKResource::setDetails);
        connect(transaction, &PackageKit::Transaction::errorCode, this, &PackageKitResource::failedFetchingDetails);
    }

    if (PackageKit::Daemon::roles() & PackageKit::Transaction::RoleGetFilesLocal) {
        PackageKit::Transaction *transaction2 = PackageKit::Daemon::getFilesLocal(m_path.toLocalFile());
        connect(transaction2, &PackageKit::Transaction::errorCode, this, &PackageKitResource::failedFetchingDetails);
        connect(transaction2, &PackageKit::Transaction::files, this, [this](const QString & /*pkgid*/, const QStringList &files) {
            const auto isFileInstalled = [](const QString &file) {
                return QFileInfo::exists(QLatin1Char('/') + file);
            };
            const bool allFilesInstalled = std::all_of(files.constBegin(), files.constEnd(), isFileInstalled);

            // PackageKit can't tell us if a package coming from the a file is installed or not,
            // so we inspect the files it wants to install and check if they're available on our running system
            setState(allFilesInstalled ? Installed : None);
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
    } else {
        // If we don't get to know the installed files, assume it's not so at least it can be installed
        setState(None);
    }
}

void LocalFilePKResource::setDetails(const PackageKit::Details &details)
{
    addPackageId(PackageKit::Transaction::InfoAvailable, details.packageId(), true);
    PackageKitResource::setDetails(details);
}

void LocalFilePKResource::invokeApplication() const
{
    KService::Ptr service = KService::Ptr(new KService(m_exec));

    if (!service) {
        return;
    }

    runService(service);
}

void LocalFilePKResource::setState(AbstractResource::State state)
{
    if (m_state == state)
        return;

    m_state = state;
    Q_EMIT stateChanged();
}
