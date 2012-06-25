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

#include "OCSResource.h"
#include <KLocale>
#include <KGlobal>

OCSResource::OCSResource(const Attica::Content& content, QObject* parent)
    : AbstractResource(parent)
    , m_content(content)
{

}

QString OCSResource::name()
{
    return m_content.name();
}

QString OCSResource::comment()
{
    QList< Attica::Icon > icons = m_content.icons();
    
    return icons.isEmpty() ? "" : icons.first().url().toString();
}

QString OCSResource::packageName() const
{
    return m_content.id();
}

QString OCSResource::icon() const
{
    return QString();
}

AbstractResource::State OCSResource::state()
{
    return None;
}

QString OCSResource::categories()
{
    return QString("ocs");
}

QUrl OCSResource::homepage() const
{
    return QString();
}

QUrl OCSResource::thumbnailUrl()
{
    return m_content.smallPreviewPicture();
}

QUrl OCSResource::screenshotUrl()
{
    return m_content.previewPicture();
}

QString OCSResource::origin() const
{
    return m_content.detailpage().host();
}

QString OCSResource::availableVersion() const
{
    return m_content.version();
}

QString OCSResource::installedVersion() const
{
    return QString("unknown");
}

QString OCSResource::sizeDescription()
{
    return KGlobal::locale()->formatByteSize(m_content.downloadUrlDescription(0).size());
}


