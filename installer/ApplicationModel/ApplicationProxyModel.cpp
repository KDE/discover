/***************************************************************************
 *   Copyright © 2010 Jonathan Thomas <echidnaman@kubuntu.org>             *
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

#include "ApplicationProxyModel.h"

#include <LibQApt/Backend>

// Own includes
#include "../Application.h"
#include "ApplicationModel.h"
#include <QDebug>

ApplicationProxyModel::ApplicationProxyModel(QObject *parent)
    : QSortFilterProxyModel(parent)
    , m_backend(0)
    , m_stateFilter((QApt::Package::State)0)
    , m_sortByRelevancy(false)
    , m_showTechnical(false)
{
}

ApplicationProxyModel::~ApplicationProxyModel()
{
}

void ApplicationProxyModel::setBackend(QApt::Backend *backend)
{
    m_backend = backend;
}

void ApplicationProxyModel::search(const QString &searchText)
{
    // 1-character searches are painfully slow. >= 2 chars are fine, though
    m_packages.clear();
    if (searchText.size() > 1) {
        m_packages = m_backend->search(searchText);
        m_sortByRelevancy = true;
    } else {
        m_sortByRelevancy = false;
    }
    invalidate();
    emit invalidated();
}

void ApplicationProxyModel::setStateFilter(QApt::Package::State state)
{
    m_stateFilter = state;
    invalidate();
    emit invalidated();
}

void ApplicationProxyModel::setOriginFilter(const QString &origin)
{
    m_originFilter = origin;
    invalidate();
    emit invalidated();
}

void ApplicationProxyModel::setFiltersFromCategory(Category *category)
{
    m_andFilters = category->andFilters();
    m_orFilters = category->orFilters();
    m_notFilters = category->notFilters();
    invalidate();
    emit invalidated();
}

void ApplicationProxyModel::setShouldShowTechnical(bool show)
{
    m_showTechnical = show;
}

bool ApplicationProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    Application *application = static_cast<ApplicationModel *>(sourceModel())->applicationAt(sourceModel()->index(sourceRow, 0, sourceParent));
    //We have a package as internal pointer
    if (!application || !application->package()) {
        return false;
    }

    if (!m_showTechnical) {
        if (application->isTechnical()) {
            return false;
        }
    }

    if (m_stateFilter) {
        if ((bool)(application->package()->state() & m_stateFilter) == false) {
            return false;
        }
    }

    if (!m_originFilter.isEmpty()) {
        if (application->package()->origin() != m_originFilter) {
            return false;
        }
    }

    if (!m_orFilters.isEmpty()) {
        // Set a boolean value to true when any of the conditions are found.
        // It is set to false by default so that if none are found, we return false
        auto filter = m_orFilters.constBegin();
        bool foundOrCondition = false;
        while (filter != m_orFilters.constEnd()) {
            switch ((*filter).first) {
            case CategoryFilter:
                if (application->categories().contains((*filter).second)) {
                    foundOrCondition = true;
                }
                break;
            case PkgSectionFilter:
                if (application->package()->latin1Section() == (*filter).second) {
                    foundOrCondition = true;
                }
                break;
            case PkgWildcardFilter: {
                QString wildcard = (*filter).second;
                wildcard.remove('*');

                if (application->package()->name().contains(wildcard)) {
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
        while (filter != m_andFilters.constEnd()) {
            switch ((*filter).first) {
            case CategoryFilter:
                if (!application->categories().contains((*filter).second)) {
                    andConditionsMet = false;
                }
                break;
            case PkgSectionFilter:
                if (!(application->package()->latin1Section() == (*filter).second)) {
                    andConditionsMet = false;
                }
                break;
            case PkgWildcardFilter: {
                QString wildcard = (*filter).second;
                wildcard.remove('*');

                if (application->package()->name().contains(wildcard)) {
                    andConditionsMet = false;
                }
            }
                break;
            case PkgNameFilter: // Only useful in the not filters
            case InvalidFilter:
            default:
                break;
            }

            ++filter;
        }

        if (!andConditionsMet) {
            return false;
        }
    }

    if (!m_notFilters.isEmpty()) {
        auto filter = m_notFilters.constBegin();
        while (filter != m_notFilters.constEnd()) {
            switch ((*filter).first) {
            case CategoryFilter:
                if (application->categories().contains((*filter).second)) {
                    return false;
                }
                break;
            case PkgSectionFilter:
                if (application->package()->latin1Section() == (*filter).second) {
                    return false;
                }
                break;
            case PkgWildcardFilter: {
                QString wildcard = (*filter).second;
                wildcard.remove('*');

                if (application->package()->name().contains(wildcard)) {
                    return false;
                }
            }
                break;
            case PkgNameFilter:
                if (application->package()->name() == (*filter).second) {
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

    if(m_sortByRelevancy) {
        return m_packages.contains(application->package());
    }

    return true;
}

Application *ApplicationProxyModel::applicationAt(const QModelIndex &index) const
{
    // Since our representation is almost bound to change, we need to grab the parent model's index
    QModelIndex sourceIndex = mapToSource(index);
    Application *application = static_cast<ApplicationModel *>(sourceModel())->applicationAt(sourceIndex);
    return application;
}

bool ApplicationProxyModel::lessThan(const QModelIndex &left, const QModelIndex &right) const
{
    if (m_sortByRelevancy) {
        ApplicationModel *model = static_cast<ApplicationModel *>(sourceModel());
        QApt::Package *leftPackage = model->applicationAt(left)->package();
        QApt::Package *rightPackage = model->applicationAt(right)->package();

        // This is expensive for very large datasets. It takes about 3 seconds with 30,000 packages
        // The order in m_packages is based on relevancy when returned by m_backend->search()
        // Use this order to determine less than
        return m_packages.indexOf(leftPackage) > m_packages.indexOf(rightPackage);
    }
    QVariant leftValue = left.data(sortRole());
    QVariant rightValue = right.data(sortRole());
    
    if(leftValue.type()==QVariant::String && rightValue.type()==QVariant::String)
        return QString::localeAwareCompare(leftValue.toString(), rightValue.toString()) < 0;
    else
        return QSortFilterProxyModel::lessThan(left, right);
}

QApt::Package::State ApplicationProxyModel::stateFilter() const
{
    return m_stateFilter;
}
