/***************************************************************************
 *   Copyright © 2013 Lukas Appelhans <boom1992@chakra-project.org>        *
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
#include "AkabeiResource.h"
#include "AkabeiBackend.h"
#include <QtCore/QStringList>
#include <akabeicore/akabeidatabase.h>
#include <akabeiquery.h>

AkabeiResource::AkabeiResource(Akabei::Package * pkg, AkabeiBackend * parent)
  : AbstractResource(parent),
    m_pkg(0),
    m_installedPkg(0)
{
    addPackage(pkg);
}
        
QString AkabeiResource::packageName() const
{
    return m_pkg->name();
}
        
QString AkabeiResource::name()
{
    return m_pkg->name();
}
        
QString AkabeiResource::comment()
{
    return m_pkg->description();
}
        
QString AkabeiResource::icon() const
{
    return "akabei";
}
        
bool AkabeiResource::canExecute() const
{
    return false;
}
        
void AkabeiResource::invokeApplication() const
{
}

void AkabeiResource::addPackage(Akabei::Package* pkg)
{
    if (pkg->database() == Akabei::Backend::instance()->localDatabase()) {
        if (!m_installedPkg || m_installedPkg->version() <= pkg->version())
            m_installedPkg = pkg;
        if (!m_pkg)
            m_pkg = pkg;
    } else if (!m_pkg || m_pkg->version() <= pkg->version()) {
        m_pkg = pkg;
    }
}
        
AbstractResource::State AkabeiResource::state()
{
    if (m_installedPkg && m_installedPkg->version() >= m_pkg->version())
        return AbstractResource::Installed;
    else if (m_installedPkg)
        return AbstractResource::Upgradeable;
    return AbstractResource::None;
}
        
QString AkabeiResource::categories()
{
    return "AudioVideo";
}
        
QUrl AkabeiResource::homepage() const
{
    return m_pkg->url();
}
        
bool AkabeiResource::isTechnical() const
{
    return false;
}

QUrl AkabeiResource::thumbnailUrl()
{
    return QUrl();
}

QUrl AkabeiResource::screenshotUrl()
{
    return QUrl();
}
        
int AkabeiResource::downloadSize()
{
    return m_pkg->size();
}

QString AkabeiResource::license()
{
    return m_pkg->licenses().join(" ");
}
        
QString AkabeiResource::installedVersion() const
{
    return m_installedPkg->version().toByteArray().data();
}

QString AkabeiResource::availableVersion() const
{
    return m_pkg->version().toByteArray().data();
}

QString AkabeiResource::longDescription() const
{
    return m_pkg->description();
}
        
QString AkabeiResource::origin() const
{
    return m_pkg->database()->name();
}

QString AkabeiResource::section()
{
    return "multimedia";
}
        
QString AkabeiResource::mimetypes() const
{
    return QString();
}
        
QList<PackageState> AkabeiResource::addonsInformation()
{
    return QList<PackageState>();
}

bool AkabeiResource::isFromSecureOrigin() const
{
    return true;
}
        
QStringList AkabeiResource::executables() const
{
    return QStringList();
}

void AkabeiResource::fetchScreenshots() {}
void AkabeiResource::fetchChangelog() {}

