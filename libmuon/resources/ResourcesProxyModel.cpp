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

ResourcesProxyModel::ResourcesProxyModel(QObject *parent)
    : QSortFilterProxyModel(parent)
    , m_sortByRelevancy(false)
    , m_filterBySearch(false)
    , m_filteredCategory(0)
    , m_stateFilter(AbstractResource::Broken)
{
    setShouldShowTechnical(false);
}

ResourcesProxyModel::~ResourcesProxyModel()
{
}

void ResourcesProxyModel::search(const QString &searchText)
{
    // 1-character searches are painfully slow. >= 2 chars are fine, though
    m_searchResults.clear();
    if (searchText.size() > 1) {
        m_lastSearch = searchText;
        
        ResourcesModel* model = qobject_cast<ResourcesModel*>(sourceModel());
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
    emit invalidated();
}

void ResourcesProxyModel::refreshSearch()
{
    search(m_lastSearch);
}

void ResourcesProxyModel::setOriginFilter(const QString &origin)
{
    if(origin.isEmpty())
        m_roleFilters.remove(ResourcesModel::OriginRole);
    else
        m_roleFilters.insert(ResourcesModel::OriginRole, origin);

    invalidateFilter();
    emit invalidated();
}

QString ResourcesProxyModel::originFilter() const
{
    return m_roleFilters.value(ResourcesModel::OriginRole).toString();
}

void ResourcesProxyModel::setFiltersFromCategory(Category *category)
{
    if(category) {
        m_andFilters = category->andFilters();
        m_orFilters = category->orFilters();
        m_notFilters = category->notFilters();
    } else {
        m_andFilters.clear();
        m_orFilters.clear();
        m_notFilters.clear();
    }

    m_filteredCategory = category;
    invalidate();
    emit invalidated();
    emit categoryChanged();
}

void ResourcesProxyModel::setShouldShowTechnical(bool show)
{
    if(!show)
        m_roleFilters.insert(ResourcesModel::IsTechnicalRole, false);
    else
        m_roleFilters.remove(ResourcesModel::IsTechnicalRole);
    invalidate();
    emit invalidated();
}

bool ResourcesProxyModel::shouldShowTechnical() const
{
    return !m_roleFilters.contains(ResourcesModel::IsTechnicalRole);
}

bool shouldFilter(const QModelIndex& idx, const QPair<FilterType, QString>& filter)
{
    bool ret = true;
    switch (filter.first) {
        case CategoryFilter:
            ret = idx.data(ResourcesModel::CategoryRole).toString().contains(filter.second);
            break;
        case PkgSectionFilter:
            ret = idx.data(ResourcesModel::SectionRole).toString() == filter.second;
            break;
        case PkgWildcardFilter: {
            QString wildcard = filter.second;
            wildcard.remove('*');
            ret = idx.data(ResourcesModel::PackageNameRole).toString().contains(wildcard);
        }   break;
        case PkgNameFilter: // Only useful in the not filters
        case InvalidFilter:
            break;
    }
    return ret;
}

bool ResourcesProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    QModelIndex idx = sourceModel()->index(sourceRow, 0, sourceParent);
    if(!idx.isValid())
        return false;
    
    for(QHash<int, QVariant>::const_iterator it=m_roleFilters.constBegin(), itEnd=m_roleFilters.constEnd(); it!=itEnd; ++it) {
        if(idx.data(it.key())!=it.value()) {
            return false;
        }
    }

    //TODO: we shouldn't even add those to the Model
//     if (application->package()->isMultiArchDuplicate())
//         return false;
    
    if(idx.data(ResourcesModel::StateRole).toInt()<m_stateFilter)
        return false;
    if(!m_filteredMimeType.isEmpty() && !idx.data(ResourcesModel::MimeTypes).toString().contains(m_filteredMimeType)) {
        return false;
    }

    if (!m_orFilters.isEmpty()) {
        bool orValue = false;
        for (QList<QPair<FilterType, QString> >::const_iterator filter = m_orFilters.constBegin(); filter != m_orFilters.constEnd(); ++filter) {
            if(shouldFilter(idx, *filter)) {
                orValue = true;
                break;
            }
        }
        if(!orValue)
            return false;
    }

    if (!m_andFilters.isEmpty()) {
        for (QList<QPair<FilterType, QString> >::const_iterator filter = m_andFilters.constBegin(); filter != m_andFilters.constEnd(); ++filter) {
            if(!shouldFilter(idx, *filter))
                return false;
        }
    }

    if (!m_notFilters.isEmpty()) {
        for(QList<QPair<FilterType, QString> >::const_iterator filter = m_notFilters.constBegin(); filter != m_notFilters.constEnd(); ++filter) {
            bool value = true;
            if(filter->first==PkgNameFilter)
                value = idx.data(ResourcesModel::PackageNameRole) == filter->second;
            else
                value = !shouldFilter(idx, *filter);
            
            if(!value)
                return false;
        }
    }

    if(m_filterBySearch) {
        return m_searchResults.contains(idx.data(ResourcesModel::PackageNameRole).toString());
    }

    return true;
}

bool ResourcesProxyModel::lessThan(const QModelIndex &left, const QModelIndex &right) const
{
    if (m_sortByRelevancy) {
        QString leftPackageName = left.data(ResourcesModel::PackageNameRole).toString();
        QString rightPackageName = right.data(ResourcesModel::PackageNameRole).toString();

        // This is expensive for very large datasets. It takes about 3 seconds with 30,000 packages
        // The order in m_packages is based on relevancy when returned by m_backend->search()
        // Use this order to determine less than
        return m_searchResults.indexOf(leftPackageName) < m_searchResults.indexOf(rightPackageName);
    }
    QVariant leftValue = left.data(sortRole());
    QVariant rightValue = right.data(sortRole());
    
    bool invert = false;
    //if we're comparing two equal values, we want the model sorted by application name
    if(sortRole()!=ResourcesModel::NameRole && leftValue == rightValue) {
        leftValue = left.data(ResourcesModel::NameRole);
        rightValue = right.data(ResourcesModel::NameRole);
        invert = (sortOrder()==Qt::DescendingOrder);
    }
    
    if(leftValue.type()==QVariant::String && rightValue.type()==QVariant::String) {
        int comp = QString::localeAwareCompare(leftValue.toString(), rightValue.toString());
        return (comp < 0) ^ invert;
    } else
        return QSortFilterProxyModel::lessThan(left, right);
}

Category* ResourcesProxyModel::filteredCategory() const
{
    return m_filteredCategory;
}

void ResourcesProxyModel::setSortByRelevancy(bool sort)
{
    m_sortByRelevancy = sort;
}

bool ResourcesProxyModel::sortingByRelevancy() const
{
    return m_sortByRelevancy;
}

bool ResourcesProxyModel::isFilteringBySearch() const
{
    return m_filterBySearch;
}

void ResourcesProxyModel::setStateFilter(AbstractResource::State s)
{
    m_stateFilter = s;
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
}
