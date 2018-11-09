/***************************************************************************
 *   Copyright © 2016 Aleix Pol Gonzalez <aleixpol@blue-systems.com>       *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU General Public License as        *
 *   published by the Free Software Foundation; either version 2 of        *
 *   the License or (at your option) version 3 or any later version        *
 *   accepted by the membership of KDE e.V. (or its successor approved     *
 *   by the membership of KDE e.V.), which shall act as a proxy            *
 *   defined in Section 14 of version 3 of the license.                    *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>. *
 ***************************************************************************/

#include "LocalFilePKResource.h"
#include <QDebug>
#include <QFileInfo>
#include <QProcess>
#include <PackageKit/Daemon>
#include <PackageKit/Details>
#include <utils.h>
#include "config-paths.h"

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
    m_details.insert(QStringLiteral("fetching"), true);//we add an entry so it's not re-fetched.

    PackageKit::Transaction* transaction = PackageKit::Daemon::getDetailsLocal(m_path.toLocalFile());
    connect(transaction, &PackageKit::Transaction::details, this, [this] (const PackageKit::Details &details){ setDetails(details); });
    connect(transaction, &PackageKit::Transaction::errorCode, this, &PackageKitResource::failedFetchingDetails);

    PackageKit::Transaction* transaction2 = PackageKit::Daemon::getFilesLocal(m_path.toLocalFile());
    connect(transaction2, &PackageKit::Transaction::errorCode, this, &PackageKitResource::failedFetchingDetails);
    connect(transaction2, &PackageKit::Transaction::files, this, [this] (const QString &/*pkgid*/, const QStringList & files){
        const auto execs = kFilter<QVector<QString>>(files, [](const QString& file) { return file.endsWith(QLatin1String(".desktop")) && file.contains(QLatin1String("usr/share/applications")); });
        if (!execs.isEmpty())
            m_exec = execs.constFirst();
        else
            qWarning() << "could not find an executable desktop file for" << m_path << "among" << files;
    });
    connect(transaction2, &PackageKit::Transaction::finished, this, [] {qDebug() << "."; });
}

QString LocalFilePKResource::license()
{
    return m_details.license();
}

void LocalFilePKResource::invokeApplication() const
{
    QProcess::startDetached(QStringLiteral(CMAKE_INSTALL_FULL_LIBEXECDIR_KF5 "/discover/runservice"), {m_exec});
}
