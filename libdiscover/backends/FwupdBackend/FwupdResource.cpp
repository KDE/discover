/***************************************************************************
 *   Copyright © 2013 Aleix Pol Gonzalez <aleixpol@blue-systems.com>       *
 *   Copyright © 2018 Abhijeet Sharma <sharma.abhijeet2096@gmail.com>      *
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

#include "FwupdResource.h"


#include <Transaction/AddonList.h>
#include <QDesktopServices>
#include <QStringList>
#include <QTimer>

FwupdResource::FwupdResource(QString name, bool isTechnical, AbstractResourcesBackend* parent)
    : AbstractResource(parent)
    , m_name(std::move(name))
    , m_state(State::Broken)
    , m_isTechnical(isTechnical)
{
    setObjectName(m_name);
}

QList<PackageState> FwupdResource::addonsInformation()
{
    return m_addons;
}

QString FwupdResource::availableVersion() const
{
    return m_version;
}

QStringList FwupdResource::allResourceNames() const
{
    return { m_name };
}

QStringList FwupdResource::categories()
{
   return m_categories;
}

QString FwupdResource::comment()
{
    return m_summary;
}

int FwupdResource::size()
{
    return m_size;
}

QUrl FwupdResource::homepage()
{
    return m_homepage;
}

QUrl FwupdResource::helpURL()
{
    return m_homepage;
}

QUrl FwupdResource::bugURL()
{
    return m_homepage;
}

QUrl FwupdResource::donationURL()
{
    return m_homepage;
}

QVariant FwupdResource::icon() const
{
    return  m_iconName;
}

QString FwupdResource::installedVersion() const
{
    return m_version;
}

QString FwupdResource::license()
{
    return m_license;
}

QString FwupdResource::longDescription()
{
    return m_description;
}

QString FwupdResource::name() const
{
    return m_name;
}

QString FwupdResource::vendor() const
{
    return m_vendor;
}

QString FwupdResource::origin() const
{
    return m_homepage.toString();
}

QString FwupdResource::packageName() const
{
    return m_name;
}

QString FwupdResource::section()
{
    return QStringLiteral("Firmware Updates");
}

AbstractResource::State FwupdResource::state()
{
    return m_state;
}

void FwupdResource::fetchChangelog()
{
    QString log = longDescription();
    log.replace(QLatin1Char('\n'), QLatin1String("<br />"));

    emit changelogFetched(log);
}

void FwupdResource::setState(AbstractResource::State state)
{
    if(m_state != state)
    {
        m_state = state;
        emit stateChanged();
    }

}

void FwupdResource::setAddons(const AddonList& addons)
{
    Q_FOREACH(const QString& toInstall, addons.addonsToInstall())
    {
        setAddonInstalled(toInstall, true);
    }
    Q_FOREACH(const QString& toRemove, addons.addonsToRemove())
    {
        setAddonInstalled(toRemove, false);
    }
}

void FwupdResource::setAddonInstalled(const QString& addon, bool installed)
{
    for(auto & elem : m_addons)
    {
        if(elem.name() == addon)
        {
            elem.setInstalled(installed);
        }
    }
}


void FwupdResource::invokeApplication() const
{
    qWarning() << "Not Launchable";
}

QUrl FwupdResource::url() const
{
    return m_homepage;
}

QString FwupdResource::executeLabel() const
{
    return i18n("Not Invokable");
}
