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

#include "DummyResource.h"
#include <QDesktopServices>

DummyResource::DummyResource(const QString& name, AbstractResourcesBackend* parent)
    : AbstractResource(parent)
    , m_name(name)
{
}

QList<PackageState> DummyResource::addonsInformation()
{
    return QList<PackageState>();
}

QString DummyResource::availableVersion() const
{
    return "3.0";
}

QString DummyResource::categories()
{
    return "dummy";
}

QString DummyResource::comment()
{
    return "comment "+name()+"...";
}

int DummyResource::downloadSize()
{
    return 123;
}

QUrl DummyResource::homepage() const
{
    return QUrl("http://kde.org");
}

QString DummyResource::icon() const
{
    return "kalarm";
}

QString DummyResource::installedVersion() const
{
    return "2.3";
}

QString DummyResource::license()
{
    return "GPL";
}

QString DummyResource::longDescription() const
{
    return "aaaaaaaaaaaaaa aaaaaaaaa aaaaaaaaaa";
}

QString DummyResource::name()
{
    return m_name;
}

QString DummyResource::origin() const
{
    return "dummy";
}

QString DummyResource::packageName() const
{
    return m_name;
}

QUrl DummyResource::screenshotUrl()
{
    return QUrl();
}

QUrl DummyResource::thumbnailUrl()
{
    return QUrl();
}

QString DummyResource::section()
{
    return QString();
}

AbstractResource::State DummyResource::state()
{
    return m_state;
}

void DummyResource::fetchChangelog()
{
    emit changelogFetched(QString());
}

void DummyResource::setState(AbstractResource::State state)
{
    m_state = state;
    emit stateChanged();
}
void DummyResource::invokeApplication() const
{
    QDesktopServices d;
    d.openUrl(QUrl("https://projects.kde.org/projects/extragear/sysadmin/muon"));
}
