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
#include <Category/CategoryModel.h>
#include <KLocalizedString>
#include <KFormat>
#include <QList>
#include <QDebug>

AbstractResource::AbstractResource(AbstractResourcesBackend* parent)
    : QObject(parent)
{
    connect(this, &AbstractResource::stateChanged, this, &AbstractResource::reportNewState);
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

QStringList AbstractResource::extends() const
{
    return {};
}

QString AbstractResource::appstreamId() const
{
    return {};
}

void AbstractResource::reportNewState()
{
    if (backend()->isFetching())
        return;

    emit backend()->resourcesChanged(this, {"state", "status", "canUpgrade", "size", "sizeDescription", "installedVersion", "availableVersion" });
}

static bool shouldFilter(AbstractResource* res, const QPair<FilterType, QString>& filter)
{
    bool ret = true;
    switch (filter.first) {
        case CategoryFilter:
            ret = res->categories().contains(filter.second);
            break;
        case PkgSectionFilter:
            ret = res->section() == filter.second;
            break;
        case PkgWildcardFilter: {
            QString wildcard = filter.second;
            wildcard.remove(QLatin1Char('*'));
            ret = res->packageName().contains(wildcard);
        }   break;
        case PkgNameFilter: // Only useful in the not filters
            ret = res->packageName() == filter.second;
            break;
        case InvalidFilter:
            break;
    }
    return ret;
}

bool AbstractResource::categoryMatches(Category* cat)
{
    {
        const auto orFilters = cat->orFilters();
        bool orValue = orFilters.isEmpty();
        for (const auto& filter: orFilters) {
            if(shouldFilter(this, filter)) {
                orValue = true;
                break;
            }
        }
        if(!orValue)
            return false;
    }

    Q_FOREACH (const auto &filter, cat->andFilters()) {
        if(!shouldFilter(this, filter))
            return false;
    }

    Q_FOREACH (const auto &filter, cat->notFilters()) {
        if(shouldFilter(this, filter))
            return false;
    }
    return true;
}

static QStringList walkCategories(AbstractResource* res, const QVector<Category*>& cats)
{
    QStringList ret;
    foreach (Category* cat, cats) {
        if (res->categoryMatches(cat)) {
            const auto subcats = walkCategories(res, cat->subCategories());
            if (subcats.isEmpty()) {
                ret += cat->name();
            } else {
                ret += subcats;
            }
        }
    }

    return ret;
}

QString AbstractResource::categoryDisplay() const
{
    const auto matchedCategories = walkCategories(const_cast<AbstractResource*>(this), CategoryModel::rootCategories().toVector());
    return matchedCategories.join(QStringLiteral(", "));
}
