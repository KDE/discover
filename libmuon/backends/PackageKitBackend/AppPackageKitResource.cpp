/***************************************************************************
 *   Copyright © 2013 Aleix Pol Gonzalez <aleixpol@blue-systems.com>       *
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

#include "AppPackageKitResource.h"
#include <KLocalizedString>
#include <KToolInvocation>
#include <QDebug>

AppPackageKitResource::AppPackageKitResource(const Appstream::Component& data, PackageKitBackend* parent)
    : PackageKitResource(data.packageNames().first(), QString(), parent)
    , m_appdata(data)
{
    Q_ASSERT(data.isValid());
}

QString AppPackageKitResource::name()
{
    return m_appdata.name();
}

QString AppPackageKitResource::longDescription()
{
    return m_appdata.description();
}

QString AppPackageKitResource::icon() const
{
    QString anIcon = m_appdata.icon();
    if (anIcon.isEmpty()) {
        QUrl iconUrl = m_appdata.iconUrl(QSize());
        if (iconUrl.isLocalFile())
            anIcon = iconUrl.toLocalFile();
    }
    return anIcon;
}

QString AppPackageKitResource::license()
{
    return m_appdata.projectLicense();
}

QStringList AppPackageKitResource::mimetypes() const
{
//     TODO
//     return m_appdata.mimetypes;
    return QStringList();
}

QStringList AppPackageKitResource::categories()
{
    return m_appdata.categories();
}

QString AppPackageKitResource::comment()
{
    return m_appdata.summary();
}

QUrl AppPackageKitResource::homepage()
{
    QList< QUrl > urls = m_appdata.urls(Appstream::Component::UrlKindHomepage);
    return urls.isEmpty() ? PackageKitResource::homepage() : urls.first();
}

bool AppPackageKitResource::isTechnical() const
{
    return false;
}

QStringList AppPackageKitResource::executables() const
{
    QStringList ret;
    for(Appstream::Provides p : m_appdata.provides())
        if (p.kind() == Appstream::Provides::KindBinary)
            ret += p.value();
    return ret;
}

void AppPackageKitResource::invokeApplication() const
{
    QStringList exes = executables();
    if(!exes.isEmpty())
        KToolInvocation::startServiceByDesktopPath(exes.first());
}
