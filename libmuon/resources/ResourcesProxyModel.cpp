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

bool ResourcesProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    QModelIndex idx = sourceModel()->index(sourceRow, 0, sourceParent);
    if(!idx.isValid())
        return false;
    
    for(QHash<int, QVariant>::const_iterator it=m_roleFilters.constBegin(), itEnd=m_roleFilters.constEnd(); it!=itEnd; ++it) {
        if(idx.data(it.key())!=it.value())
            return false;
    }

    //TODO: we shouldn't even add those to the Model
//     if (application->package()->isMultiArchDuplicate())
//         return false;

    if (!m_orFilters.isEmpty()) {
        // Set a boolean value to true when any of the conditions are found.
        // It is set to false by default so that if none are found, we return false
        auto filter = m_orFilters.constBegin();
        bool foundOrCondition = false;
        while (filter != m_orFilters.constEnd()) {
            switch (filter->first) {
            case CategoryFilter:
                if (idx.data(ResourcesModel::CategoryRole).toString().contains(filter->second)) {
                    foundOrCondition = true;
                }
                break;
            case PkgSectionFilter:
                if (idx.data(ResourcesModel::SectionRole).toString() == filter->second) {
                    foundOrCondition = true;
                }
                break;
            case PkgWildcardFilter: {
                QString wildcard = filter->second;
                wildcard.remove('*');

                if (idx.data(ResourcesModel::PackageNameRole).toString().contains(wildcard)) {
                    foundOrCondition = true;
                }
                break;
            }
            case PkgNameFilter: // Only useful in the not filters
            case InvalidFilter:
            default:
                break;
            }

            ++filter;
        }

        if (!foundOrCondition) {
            return false;
        }
    }

    if (!m_andFilters.isEmpty()) {
        // Set a boolean value to false when any conditions fail to meet
        auto filter = m_andFilters.constBegin();
        bool andConditionsMet = true;
        for (; filter != m_andFilters.constEnd() && andConditionsMet; ++filter) {
            switch (filter->first) {
            case CategoryFilter:
                if (!idx.data(ResourcesModel::CategoryRole).toString().contains(filter->second)) {
                    andConditionsMet = false;
                }
                break;
            case PkgSectionFilter:
                if (!(idx.data(ResourcesModel::SectionRole).toString() == filter->second)) {
                    andConditionsMet = false;
                }
                break;
            case PkgWildcardFilter: {
                QString wildcard = filter->second;
                wildcard.remove('*');

                if (!idx.data(ResourcesModel::PackageNameRole).toString().contains(wildcard)) {
                    andConditionsMet = false;
                }
            }
                break;
            case PkgNameFilter: // Only useful in the not filters
            case InvalidFilter:
            default:
                break;
            }

            if (!andConditionsMet) {
                return false;
            }
        }
    }

    if (!m_notFilters.isEmpty()) {
        auto filter = m_notFilters.constBegin();
        while (filter != m_notFilters.constEnd()) {
            switch (filter->first) {
            case CategoryFilter:
                if (idx.data(ResourcesModel::CategoryRole).toString().contains(filter->second)) {
                    return false;
                }
                break;
            case PkgSectionFilter:
                if (idx.data(ResourcesModel::SectionRole).toString() == filter->second) {
                    return false;
                }
                break;
            case PkgWildcardFilter: {
                QString wildcard = filter->second;
                wildcard.remove('*');

                if (idx.data(ResourcesModel::PackageNameRole).toString().contains(wildcard)) {
                    return false;
                }
            }
                break;
            case PkgNameFilter:
                if (idx.data(ResourcesModel::PackageNameRole) == filter->second) {
                    return false;
                }
                break;
            case InvalidFilter:
            default:
                break;
            }

            ++filter;
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
