/***************************************************************************
 *   Copyright © 2010 Jonathan Thomas <echidnaman@kubuntu.org>             *
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

#include "ResourcesProxyModel.h"

#include <QDebug>

#include "ResourcesModel.h"
#include "AbstractResource.h"
#include "AbstractResourcesBackend.h"
#include <Category/CategoryModel.h>

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

static bool categoryMatches(AbstractResource* res, Category* cat)
{
    {
        const auto orFilters = cat->orFilters();
        bool orValue = orFilters.isEmpty();
        for (const auto& filter: orFilters) {
            if(shouldFilter(res, filter)) {
                orValue = true;
                break;
            }
        }
        if(!orValue)
            return false;
    }

    Q_FOREACH (const auto &filter, cat->andFilters()) {
        if(!shouldFilter(res, filter))
            return false;
    }

    Q_FOREACH (const auto &filter, cat->notFilters()) {
        if(shouldFilter(res, filter))
            return false;
    }
    return true;
}

ResourcesProxyModel::ResourcesProxyModel(QObject *parent)
    : QSortFilterProxyModel(parent)
    , m_sortByRelevancy(false)
    , m_filterBySearch(false)
    , m_filteredCategory(nullptr)
    , m_stateFilter(AbstractResource::Broken)
{
    connect(ResourcesModel::global(), &ResourcesModel::searchInvalidated, this, &ResourcesProxyModel::refreshSearch);

    setShouldShowTechnical(false);

    setSourceModel(ResourcesModel::global());
}

QHash<int, QByteArray> ResourcesProxyModel::roleNames() const
{
    return ResourcesModel::global()->roleNames();
}

void ResourcesProxyModel::setSourceModel(QAbstractItemModel* source)
{
    Q_ASSERT(ResourcesModel::global() == source);
    QSortFilterProxyModel::setSourceModel(source);
}

void ResourcesProxyModel::setSearch(const QString &searchText)
{
    const bool diff = searchText != m_lastSearch;

    m_searchResults.clear();
    m_lastSearch = searchText;
    ResourcesModel* model = qobject_cast<ResourcesModel*>(sourceModel());
    if(!model) {
        return;
    }

    // 1-character searches are painfully slow. >= 2 chars are fine, though
    if (searchText.size() > 1) {
        QVector< AbstractResourcesBackend* > backends= model->backends();
        foreach(AbstractResourcesBackend* backend, backends)
            m_searchResults += backend->searchPackageName(searchText);
        m_sortByRelevancy = true;
        m_filterBySearch = true;
    } else {
        m_filterBySearch = false;
        m_sortByRelevancy = false;
    }
    invalidateFilter();

    if (diff) {
        Q_EMIT searchChanged(m_lastSearch);
        Q_EMIT subcategoriesChanged(subcategories());
    }
}

QString ResourcesProxyModel::lastSearch() const
{
    return m_lastSearch;
}

void ResourcesProxyModel::refreshSearch()
{
    setSearch(lastSearch());
}

void ResourcesProxyModel::setOriginFilter(const QString &origin)
{
    if (origin == originFilter())
        return;

    if(origin.isEmpty())
        m_roleFilters.remove("origin");
    else
        m_roleFilters.insert("origin", origin);

    invalidateFilter();
}

QString ResourcesProxyModel::originFilter() const
{
    return m_roleFilters.value("origin").toString();
}

void ResourcesProxyModel::setFiltersFromCategory(Category *category)
{
    if(category==m_filteredCategory)
        return;

    m_filteredCategory = category;
    invalidateFilter();
    emit categoryChanged();
    emit subcategoriesChanged(subcategories());
}

QList<Category *> ResourcesProxyModel::subcategories() const
{
    const auto cats = m_filteredCategory ? m_filteredCategory->subCategories().toList() : CategoryModel::rootCategories();

    const int count = rowCount();
    QSet<Category *> ret;
    foreach(Category* cat, cats) {
        if (ret.contains(cat))
            continue;

        for (int i=0; i<count; ++i) {
            AbstractResource* res = qobject_cast<AbstractResource*>(index(i, 0).data(ResourcesModel::ApplicationRole).value<QObject*>());
            if (categoryMatches(res, cat)) {
                ret.insert(cat);
                break;
            }
        }
    }
    return ret.toList();
}

void ResourcesProxyModel::setShouldShowTechnical(bool show)
{
    if (shouldShowTechnical() == show)
        return;

    if(!show)
        m_roleFilters.insert("isTechnical", false);
    else
        m_roleFilters.remove("isTechnical");
    emit showTechnicalChanged();
    invalidateFilter();
}

bool ResourcesProxyModel::shouldShowTechnical() const
{
    return !m_roleFilters.contains("isTechnical");
}

bool ResourcesProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    AbstractResource* res = qobject_cast<AbstractResource*>(sourceModel()->index(sourceRow, 0, sourceParent).data(ResourcesModel::ApplicationRole).value<QObject*>());
    if(!res) //assert?
        return false;

    if(!m_extends.isEmpty() && !res->extends().contains(m_extends)) {
        return false;
    }

    if(m_filterBySearch && !m_searchResults.contains(res)) {
        return false;
    }

    for(QHash<QByteArray, QVariant>::const_iterator it=m_roleFilters.constBegin(), itEnd=m_roleFilters.constEnd(); it!=itEnd; ++it) {
        Q_ASSERT(AbstractResource::staticMetaObject.indexOfProperty(it.key().constData())>=0);
        if(res->property(it.key().constData())!=it.value()) {
            return false;
        }
    }

    if(res->state() < m_stateFilter)
        return false;

    if(!m_filteredMimeType.isEmpty() && !res->mimetypes().contains(m_filteredMimeType)) {
        return false;
    }

    return !m_filteredCategory || categoryMatches(res, m_filteredCategory);
}

bool ResourcesProxyModel::lessThan(const QModelIndex &left, const QModelIndex &right) const
{
    if (m_sortByRelevancy) {
        AbstractResource* leftPackage = qobject_cast<AbstractResource*>(left.data(ResourcesModel::ApplicationRole).value<QObject*>());
        AbstractResource* rightPackage = qobject_cast<AbstractResource*>(right.data(ResourcesModel::ApplicationRole).value<QObject*>());

        // This is expensive for very large datasets. It takes about 3 seconds with 30,000 packages
        // The order in m_packages is based on relevancy when returned by m_backend->search()
        // Use this order to determine less than
        Q_FOREACH (AbstractResource* res, m_searchResults) {
            if(res == leftPackage)
                return true;
            else if(res == rightPackage)
                return false;
        }
    }

    int theSortRole = sortRole();
    
    bool invert = false;
    //if we're comparing two equal values, we want the model sorted by application name
    if(theSortRole != ResourcesModel::NameRole) {
        QVariant leftValue = left.data(theSortRole);
        QVariant rightValue = right.data(theSortRole);
        if (leftValue == rightValue) {
            theSortRole = ResourcesModel::NameRole;
            invert = (sortOrder()==Qt::DescendingOrder);
        }
    }
    
    if(theSortRole == ResourcesModel::NameRole) {
        AbstractResource* leftPackage = qobject_cast<AbstractResource*>(left.data(ResourcesModel::ApplicationRole).value<QObject*>());
        AbstractResource* rightPackage = qobject_cast<AbstractResource*>(right.data(ResourcesModel::ApplicationRole).value<QObject*>());

        return (leftPackage->nameSortKey().compare(rightPackage->nameSortKey())<0) ^ invert;
    } else if(theSortRole == ResourcesModel::CanUpgrade) {
        QVariant leftValue = left.data(theSortRole);
        return leftValue.toBool();
    } else {
        return QSortFilterProxyModel::lessThan(left, right) ^ invert;
    }
}

Category* ResourcesProxyModel::filteredCategory() const
{
    return m_filteredCategory;
}

void ResourcesProxyModel::setSortByRelevancy(bool sort)
{
    m_sortByRelevancy = sort;
    invalidate();
}

bool ResourcesProxyModel::sortingByRelevancy() const
{
    return m_sortByRelevancy;
}

void ResourcesProxyModel::setStateFilter(AbstractResource::State s)
{
    m_stateFilter = s;
    invalidateFilter();
    emit stateFilterChanged();
}

AbstractResource::State ResourcesProxyModel::stateFilter() const
{
    return m_stateFilter;
}

QString ResourcesProxyModel::mimeTypeFilter() const
{
    return m_filteredMimeType;
}

void ResourcesProxyModel::setMimeTypeFilter(const QString& mime)
{
    m_filteredMimeType = mime;
    invalidateFilter();
}

QString ResourcesProxyModel::extends() const
{
    return m_extends;
}



void ResourcesProxyModel::setExtends(const QString& extends)
{
    m_extends = extends;
    invalidateFilter();
}
