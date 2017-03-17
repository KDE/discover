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

QString LocalFilePKResource::name()
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
