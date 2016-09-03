/***************************************************************************
 *   Copyright © 2012 Aleix Pol Gonzalez <aleixpol@blue-systems.com>       *
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

#include "KNSResource.h"
#include "KNSBackend.h"
#include <QDebug>
#include <knewstuff_version.h>

KNSResource::KNSResource(const KNS3::Entry& entry, QString  category, KNSBackend* parent)
    : AbstractResource(parent)
    , m_category(std::move(category))
    , m_entry(entry)
{
    connect(this, &KNSResource::stateChanged, parent, &KNSBackend::updatesCountChanged);
}

KNSResource::~KNSResource() = default;

AbstractResource::State KNSResource::state()
{
    switch(m_entry.status()) {
        case KNS3::Entry::Invalid:
            return Broken;
        case KNS3::Entry::Downloadable:
            return None;
        case KNS3::Entry::Installed:
            return Installed;
        case KNS3::Entry::Updateable:
            return Upgradeable;
        case KNS3::Entry::Deleted:
        case KNS3::Entry::Installing:
        case KNS3::Entry::Updating:
            return None;
    }
    return None;
}

QVariant KNSResource::icon() const
{
    return qobject_cast<KNSBackend*>(parent())->iconName();
}

QString KNSResource::comment()
{
    QString s = m_entry.shortSummary();
    if(s.isEmpty()) {
        s = longDescription();
        int newLine = s.indexOf(QLatin1Char('\n'));
        if(newLine>0)
            s=s.left(newLine);
    }
    return s;
}

QString KNSResource::name()
{
    return m_entry.name();
}

QString KNSResource::packageName() const
{
    return m_entry.id();
}

QStringList KNSResource::categories()
{
    return QStringList(m_category);
}

QUrl KNSResource::homepage()
{
    return m_entry.url();
}

template <class T>
static T firstIfExists(const QList<T> &list)
{
    return list.isEmpty() ? T() : list.at(0);
}

QUrl KNSResource::thumbnailUrl()
{
    return firstIfExists(m_entry.previewThumbnails());
}

QUrl KNSResource::screenshotUrl()
{
    return firstIfExists(m_entry.previewImages());
}

QString KNSResource::longDescription()
{
    QString ret = m_entry.summary();
    ret = ret.replace(QLatin1Char('\r'), QString());
    return ret;
}

void KNSResource::setEntry(const KNS3::Entry& entry)
{
    m_entry = entry;
    Q_EMIT stateChanged();
}

KNS3::Entry KNSResource::entry() const
{
    return m_entry;
}

QString KNSResource::license()
{
    return m_entry.license();
}

int KNSResource::size()
{
    return m_entry.size();
}

QString KNSResource::installedVersion() const
{
    return m_entry.version();
}

QString KNSResource::availableVersion() const
{
    return m_entry.updateVersion();
}

QString KNSResource::origin() const
{
    return m_entry.providerId();
}

QString KNSResource::section()
{
    return m_entry.category();
}

void KNSResource::fetchScreenshots()
{
    emit screenshotsFetched(m_entry.previewThumbnails(), m_entry.previewImages());
}

void KNSResource::fetchChangelog()
{
    emit changelogFetched(m_entry.changelog());
}

QStringList KNSResource::extends() const
{
    return qobject_cast<KNSBackend*>(parent())->extends();
}
