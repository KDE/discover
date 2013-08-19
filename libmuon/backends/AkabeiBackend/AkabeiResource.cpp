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
#include <akabeigroup.h>
#include <kdebug.h>
#include <MuonDataSources.h>

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
    return false;//FIXME
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
    //if (s != state()) //FIXME: Save the old state before callin clearPackages
        emit stateChanged();
}

void AkabeiResource::clearPackages()
{
    m_pkg = 0;
    m_installedPkg = 0;
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
    return "Unknown";//FIXME: Has to be added to akabei
}
        
QUrl AkabeiResource::homepage() const
{
    return m_pkg->url();
}
        
bool AkabeiResource::isTechnical() const
{
    return true;
}

QUrl AkabeiResource::thumbnailUrl()
{
    return KUrl(MuonDataSources::screenshotsSource(), "thumbnail/"+packageName());//FIXME: Also return the packages screenshot, probably as priority or fallback?
}

QUrl AkabeiResource::screenshotUrl()
{
    //if (m_pkg && !m_pkg->screenshot().isEmpty()) {
    //    return m_pkg->screenshot();
    //}
    return KUrl(MuonDataSources::screenshotsSource(), "screenshot/"+packageName());
}
        
int AkabeiResource::downloadSize()
{
    return m_pkg->size();
}

QString AkabeiResource::license()
{
    return m_pkg->licenses().join(", ");
}
        
QString AkabeiResource::installedVersion() const
{
    if (!m_installedPkg)
        return QString();
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
    if (m_pkg->groups().isEmpty())
        return "unknown";
    return m_pkg->groups().first()->name();//FIXME: Probably add support for multiple sections?
}
        
QString AkabeiResource::mimetypes() const
{
    return QString();
}
        
QList<PackageState> AkabeiResource::addonsInformation()
{
    QList<PackageState> states;
    foreach (const QString &optdep, m_pkg->optionalDependencies()) {
        QStringList split = optdep.split(":");
        if (split.count() >= 2) {
            states.append(PackageState(split.first(), split.at(1), 
                                       !Akabei::Backend::instance()->localDatabase()->queryPackages(Akabei::Queries::selectPackages("name", "LIKE", split.first())).isEmpty()));
        }
    }
    return states;
}

bool AkabeiResource::isFromSecureOrigin() const
{
    return true;
}
        
QStringList AkabeiResource::executables() const
{
    return QStringList();
}

Akabei::Package * AkabeiResource::package() const
{
    return m_pkg;
}

Akabei::Package * AkabeiResource::installedPackage() const
{
    return m_installedPkg;
}

void AkabeiResource::fetchScreenshots() {}
void AkabeiResource::fetchChangelog() {}

