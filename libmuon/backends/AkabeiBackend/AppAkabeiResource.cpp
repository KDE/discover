/***************************************************************************
 *   Copyright © 2013 Aleix Pol Gonzalez <aleixpol@blue-systems.com>       *
 *   Copyright © 2013 Lukas Appelhans <l.appelhans@gmx.de>                 *
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

#include "AppAkabeiResource.h"
#include <KGlobal>
#include <KLocale>
#include <KStandardDirs>
#include <KToolInvocation>
#include <QDebug>
#include <akabeipackage.h>

AppPackageKitResource::AppPackageKitResource(const ApplicationData& data,
                                             Akabei::Package * pkg,
                                             AkabeiBackend* parent)
    : AkabeiResource(pkg, parent)
    , m_appdata(data)
{}

QString AppPackageKitResource::name()
{
    QString ret = m_appdata.name.value(KGlobal::locale()->language());
    if(ret.isEmpty()) ret = m_appdata.name.value(QString());
    if(ret.isEmpty()) ret = m_appdata.pkgname;
    if(ret.isEmpty()) ret = AkabeiResource::name();
    return ret;
}

QString AppPackageKitResource::longDescription() const
{
    QString ret = m_appdata.summary.value(KGlobal::locale()->language());
    if(ret.isEmpty()) ret = m_appdata.summary.value(QString());
    if(ret.isEmpty() || ret.length() < AkabeiResource::longDescription().length()) ret = AkabeiResource::longDescription();
    return ret;
}

QString AppPackageKitResource::icon() const
{
    return m_appdata.icon;
}

QString AppPackageKitResource::mimetypes() const
{
    return m_appdata.mimetypes.first();
}

QString AppPackageKitResource::categories()
{
    return m_appdata.appcategories.first();
}

QUrl AppPackageKitResource::homepage() const
{
    return m_appdata.url.isEmpty() ? AkabeiResource::homepage() : m_appdata.url;
}

bool AppPackageKitResource::isTechnical() const
{
    return false;
}

QStringList AppPackageKitResource::executables() const
{
    QString desktopFile = KGlobal::dirs()->findResource("xdgdata-apps", m_appdata.id);
    QStringList ret;
    if(!desktopFile.isEmpty())
        ret += desktopFile;
    return ret;
}

void AppPackageKitResource::invokeApplication() const
{
    QStringList exes = executables();
    if(!exes.isEmpty())
        KToolInvocation::startServiceByDesktopPath(exes.first());
}
