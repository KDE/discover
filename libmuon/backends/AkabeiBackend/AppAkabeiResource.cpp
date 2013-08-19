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

AppAkabeiResource::AppAkabeiResource(const ApplicationData& data,
                                             Akabei::Package * pkg,
                                             AkabeiBackend* parent)
    : AkabeiResource(pkg, parent)
    , m_appdata(data)
{}

QString AppAkabeiResource::name()
{
    QString ret = m_appdata.name.value(KGlobal::locale()->language());
    if(ret.isEmpty()) ret = m_appdata.name.value(QString());
    if(ret.isEmpty()) ret = m_appdata.pkgname;
    if(ret.isEmpty()) ret = AkabeiResource::name();
    return ret;
}

QString AppAkabeiResource::longDescription() const
{
    QString ret = m_appdata.summary.value(KGlobal::locale()->language());
    if(ret.isEmpty()) ret = m_appdata.summary.value(QString());
    if(ret.isEmpty() || ret.length() < AkabeiResource::longDescription().length()) ret = AkabeiResource::longDescription();
    return ret;
}

QString AppAkabeiResource::icon() const
{
    return m_appdata.icon;
}

QString AppAkabeiResource::mimetypes() const
{
    return m_appdata.mimetypes.first();
}

QString AppAkabeiResource::categories()
{
    return m_appdata.appcategories.first();
}

QUrl AppAkabeiResource::homepage() const
{
    return m_appdata.url.isEmpty() ? AkabeiResource::homepage() : m_appdata.url;
}

bool AppAkabeiResource::isTechnical() const
{
    return false;
}

QStringList AppAkabeiResource::executables() const
{
    QString desktopFile = KGlobal::dirs()->findResource("xdgdata-apps", m_appdata.id);
    QStringList ret;
    if(!desktopFile.isEmpty())
        ret += desktopFile;
    return ret;
}

void AppAkabeiResource::invokeApplication() const
{
    QStringList exes = executables();
    if(!exes.isEmpty())
        KToolInvocation::startServiceByDesktopPath(exes.first());
}
