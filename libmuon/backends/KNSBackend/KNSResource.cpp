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

KNSResource::KNSResource(const Attica::Content& c, const QString& category, const QString& icon, KNSBackend* parent)
    : AbstractResource(parent)
    , m_status(KNS3::Entry::Downloadable)
    , m_content(c)
    , m_category(category)
    , m_icon(icon)
    , m_entry(0)
{}

KNSResource::~KNSResource()
{
    delete m_entry;
}

AbstractResource::State KNSResource::state()
{
    switch(m_status) {
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

void KNSResource::setStatus(KNS3::Entry::Status status)
{
    if(status!=m_status) {
        m_status = status;
        emit stateChanged();
    }
}

QString KNSResource::icon() const
{
    return m_icon;
}

QString KNSResource::comment()
{
    return m_content.summary();
}

QString KNSResource::name()
{
    return m_content.name();
}

QString KNSResource::packageName() const
{
    return m_content.id();
}

QString KNSResource::categories()
{
    return m_category;
}

QUrl KNSResource::homepage() const
{
    return m_content.detailpage();
}

QUrl KNSResource::thumbnailUrl()
{
    return m_content.smallPreviewPicture();
}

QUrl KNSResource::screenshotUrl()
{
    return m_content.previewPicture();
}

Attica::Content& KNSResource::content()
{
    return m_content;
}

QString KNSResource::longDescription() const
{
    QString ret = m_content.description();
    ret = ret.replace('\r', QString());
    return ret;
}

void KNSResource::setEntry(const KNS3::Entry& entry)
{
    setStatus(entry.status());
    delete m_entry;
    m_entry = new KNS3::Entry(entry);
}

KNS3::Entry* KNSResource::entry() const
{
    return m_entry;
}

QString KNSResource::license()
{
    return m_content.licenseName();
}

int KNSResource::downloadSize()
{
    return m_content.downloadUrlDescription(0).size();
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
    return m_content.downloadUrlDescription(0).category();
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
