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

#include "AbstractResource.h"
#include "AbstractResourcesBackend.h"
#include <ReviewsBackend/AbstractReviewsBackend.h>
#include <klocalizedstring.h>
#include <KFormat>

AbstractResource::AbstractResource(AbstractResourcesBackend* parent)
    : QObject(parent)
{
    if (parent && parent->reviewsBackend())
        connect(parent->reviewsBackend(), &AbstractReviewsBackend::ratingsReady, this, &AbstractResource::ratingFetched);
}

bool AbstractResource::canExecute() const
{
    return false;
}

void AbstractResource::invokeApplication() const
{}

bool AbstractResource::isTechnical() const
{
    return false;
}

bool AbstractResource::canUpgrade()
{
    return state() == Upgradeable;
}

bool AbstractResource::isInstalled()
{
    return state() >= Installed;
}

void AbstractResource::fetchScreenshots()
{
    QList<QUrl> thumbs, screens;
    QUrl thumbnail = thumbnailUrl();
    if(!thumbnail.isEmpty()) {
        thumbs << thumbnail;
        screens << screenshotUrl();
    }
    emit screenshotsFetched(thumbs, screens);
}

QStringList AbstractResource::mimetypes() const
{
    return QStringList();
}

QStringList AbstractResource::executables() const
{
    return QStringList();
}

AbstractResourcesBackend* AbstractResource::backend() const
{
    return static_cast<AbstractResourcesBackend*>(parent());
}

QString AbstractResource::status()
{
    switch(state()) {
        case Broken: return i18n("Broken");
        case None: return i18n("Available");
        case Installed: return i18n("Installed");
        case Upgradeable: return i18n("Upgradeable");
    }
    return QString();
}

bool AbstractResource::isFromSecureOrigin() const
{
    return false;
}

QString AbstractResource::sizeDescription()
{
    return KFormat().formatByteSize(size());
}

QCollatorSortKey AbstractResource::nameSortKey()
{
    if (!m_collatorKey) {
        m_collatorKey.reset(new QCollatorSortKey(QCollator().sortKey(name())));
    }
    return *m_collatorKey;
}

Rating* AbstractResource::rating() const
{
    AbstractReviewsBackend* ratings = backend()->reviewsBackend();
    return ratings ? ratings->ratingForApplication(const_cast<AbstractResource*>(this)) : nullptr;
}

QString AbstractResource::appstreamId() const
{
    return {};
}
