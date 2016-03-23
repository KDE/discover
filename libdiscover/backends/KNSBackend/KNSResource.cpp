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

KNSResource::KNSResource(const Attica::Content& c, QString  category, KNSBackend* parent)
    : AbstractResource(parent)
    , m_content(c)
    , m_category(std::move(category))
    , m_entry(nullptr)
{
}

KNSResource::~KNSResource()
{
}

AbstractResource::State KNSResource::state()
{
    if (!m_entry)
        return None;
    switch(m_entry->status()) {
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

QString KNSResource::icon() const
{
    return qobject_cast<KNSBackend*>(parent())->iconName();
}

QString KNSResource::comment()
{
    QString s = m_content.summary();
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
    return m_content.name();
}

QString KNSResource::packageName() const
{
    return m_content.id();
}

QStringList KNSResource::categories()
{
    return QStringList(m_category);
}

QUrl KNSResource::homepage()
{
    return m_content.detailpage();
}

QUrl KNSResource::thumbnailUrl()
{
    return QUrl(m_content.smallPreviewPicture());
}

QUrl KNSResource::screenshotUrl()
{
    return QUrl(m_content.previewPicture());
}

const Attica::Content& KNSResource::content()
{
    return m_content;
}

QString KNSResource::longDescription()
{
    QString ret = m_content.description();
    ret = ret.replace(QLatin1Char('\r'), QString());
    return ret;
}

void KNSResource::setEntry(const KNS3::Entry& entry)
{
    m_entry.reset(new KNS3::Entry(entry));
    Q_EMIT stateChanged();
}

KNS3::Entry* KNSResource::entry() const
{
    return m_entry.data();
}

QString KNSResource::license()
{
    return m_content.licenseName();
}

int KNSResource::size()
{
    const Attica::DownloadDescription desc = m_content.downloadUrlDescription(0);
    return desc.size();
}

QString KNSResource::installedVersion() const
{
    return m_entry->version();
}

QString KNSResource::availableVersion() const
{
    return m_content.version();
}

QString KNSResource::origin() const
{
    return m_entry->providerId();
}

QString KNSResource::section()
{
    const Attica::DownloadDescription desc = m_content.downloadUrlDescription(0);
    return desc.category();
}

void KNSResource::fetchScreenshots()
{
    QList<QUrl> thumbnails, screenshots;
    for(int i=0; i<=3; i++) {
        QString number = QString::number(i);
        QString last = m_content.previewPicture(number);
        if(!last.isEmpty()) {
            thumbnails += QUrl(m_content.smallPreviewPicture(number));
            screenshots += QUrl(last);
        }
    }
    emit screenshotsFetched(thumbnails, screenshots);
}

void KNSResource::fetchChangelog()
{
    emit changelogFetched(m_content.changelog());
}
