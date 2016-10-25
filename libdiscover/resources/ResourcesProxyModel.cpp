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
#include <QMetaProperty>
#include <utils.h>

#include "ResourcesModel.h"
#include "AbstractResource.h"
#include "AbstractResourcesBackend.h"
#include <Category/CategoryModel.h>
#include <ReviewsBackend/Rating.h>
#include <Transaction/TransactionModel.h>

ResourcesProxyModel::ResourcesProxyModel(QObject *parent)
    : QAbstractListModel(parent)
    , m_sortRole(ResourcesModel::NameRole)
    , m_sortOrder(Qt::AscendingOrder)
    , m_sortByRelevancy(false)
    , m_filterBySearch(false)
    , m_roles({
        { ResourcesModel::NameRole, "name" },
        { ResourcesModel::IconRole, "icon" },
        { ResourcesModel::CommentRole, "comment" },
        { ResourcesModel::StateRole, "state" },
        { ResourcesModel::RatingRole, "rating" },
        { ResourcesModel::RatingPointsRole, "ratingPoints" },
        { ResourcesModel::RatingCountRole, "ratingCount" },
        { ResourcesModel::SortableRatingRole, "sortableRating" },
        { ResourcesModel::ActiveRole, "active" },
        { ResourcesModel::InstalledRole, "isInstalled" },
        { ResourcesModel::ApplicationRole, "application" },
        { ResourcesModel::OriginRole, "origin" },
        { ResourcesModel::CanUpgrade, "canUpgrade" },
        { ResourcesModel::PackageNameRole, "packageName" },
        { ResourcesModel::IsTechnicalRole, "isTechnical" },
        { ResourcesModel::CategoryRole, "category" },
        { ResourcesModel::CategoryDisplayRole, "categoryDisplay" },
        { ResourcesModel::SectionRole, "section" },
        { ResourcesModel::MimeTypes, "mimetypes" },
        { ResourcesModel::LongDescriptionRole, "longDescription" },
        { ResourcesModel::SizeRole, "size" }
        })
    , m_currentStream(nullptr)
{
//     new ModelTest(this, this);

    connect(ResourcesModel::global(), &ResourcesModel::searchInvalidated, this, &ResourcesProxyModel::refreshSearch);
    connect(ResourcesModel::global(), &ResourcesModel::backendsChanged, this, &ResourcesProxyModel::invalidateFilter);
    connect(ResourcesModel::global(), &ResourcesModel::backendDataChanged, this, &ResourcesProxyModel::refreshBackend);
    connect(ResourcesModel::global(), &ResourcesModel::resourceDataChanged, this, &ResourcesProxyModel::refreshResource);
    connect(ResourcesModel::global(), &ResourcesModel::resourceRemoved, this, &ResourcesProxyModel::removeResource);

    connect(TransactionModel::global(), &TransactionModel::transactionAdded, this, &ResourcesProxyModel::resourceChangedByTransaction);
    connect(TransactionModel::global(), &TransactionModel::transactionRemoved, this, &ResourcesProxyModel::resourceChangedByTransaction);

    setShouldShowTechnical(false);
}

QHash<int, QByteArray> ResourcesProxyModel::roleNames() const
{
    return m_roles;
}

void ResourcesProxyModel::setSortRole(int sortRole)
{
    if (sortRole != m_sortRole) {
        Q_ASSERT(roleNames().contains(sortRole));

        m_sortRole = sortRole;
        Q_EMIT sortRoleChanged(sortRole);
        invalidateSorting();
    }
}

void ResourcesProxyModel::setSortOrder(Qt::SortOrder sortOrder)
{
    if (sortOrder != m_sortOrder) {
        m_sortRole = sortOrder;
        Q_EMIT sortRoleChanged(sortOrder);
        invalidateSorting();
    }
}

void ResourcesProxyModel::setSearch(const QString &searchText)
{
    const bool diff = searchText != m_filters.search;

    m_displayedResources.clear();
    m_filters.search = searchText;

    // 1-character searches are painfully slow. >= 2 chars are fine, though
    if (searchText.size() > 1) {
        m_sortByRelevancy = true;
        m_filterBySearch = true;
    } else {
        m_filterBySearch = false;
        m_sortByRelevancy = false;
    }
    invalidateFilter();

    if (diff) {
        Q_EMIT searchChanged(m_filters.search);
        fetchSubcategories();
    }
}

void ResourcesProxyModel::addResources(const QVector<AbstractResource *>& _res)
{
    auto res = _res;
    m_filters.filterJustInCase(res);

    if (res.isEmpty())
        return;

    beginResetModel();
    m_displayedResources += res;
    qSort(m_displayedResources.begin(), m_displayedResources.end(), [this](AbstractResource* res, AbstractResource* res2){ return lessThan(res, res2); });
    endResetModel();
}

void ResourcesProxyModel::invalidateSorting()
{
    if (m_displayedResources.isEmpty())
        return;

    beginResetModel();
    qSort(m_displayedResources.begin(), m_displayedResources.end(), [this](AbstractResource* res, AbstractResource* res2){ return lessThan(res, res2); });
    endResetModel();
}

QString ResourcesProxyModel::lastSearch() const
{
    return m_filters.search;
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
        m_filters.roles.remove("origin");
    else
        m_filters.roles.insert("origin", origin);

    invalidateFilter();
}

QString ResourcesProxyModel::originFilter() const
{
    return m_filters.roles.value("origin").toString();
}

void ResourcesProxyModel::setFiltersFromCategory(Category *category)
{
    if(category==m_filters.category)
        return;

    m_filters.category = category;
    invalidateFilter();
    emit categoryChanged();
    fetchSubcategories();
}

void ResourcesProxyModel::fetchSubcategories()
{
    const auto cats = m_filters.category ? m_filters.category->subCategories().toList() : CategoryModel::rootCategories();

    const int count = rowCount();
    QSet<Category*> done;
    for (int i=0; i<count; ++i) {
        AbstractResource* res = m_displayedResources[i];
        done.unite(res->categoryObjects());
    }
    QVariantList ret;
    foreach (Category* cat, done)
        ret += QVariant::fromValue<QObject*>(cat);

    if (ret != m_subcategories) {
        m_subcategories = ret;
        Q_EMIT subcategoriesChanged(m_subcategories);
    }
}

QVariantList ResourcesProxyModel::subcategories() const
{
    return m_subcategories;
}

void ResourcesProxyModel::setShouldShowTechnical(bool show)
{
    if (shouldShowTechnical() == show)
        return;

    if(!show)
        m_filters.roles.insert("isTechnical", false);
    else
        m_filters.roles.remove("isTechnical");
    emit showTechnicalChanged();
    invalidateFilter();
}

bool ResourcesProxyModel::shouldShowTechnical() const
{
    return !m_filters.roles.contains("isTechnical");
}

void ResourcesProxyModel::invalidateFilter()
{
    if (m_currentStream) {
        connect(this, &ResourcesProxyModel::busyChanged, this, &ResourcesProxyModel::invalidateFilter);
        return;
    }
    disconnect(this, &ResourcesProxyModel::busyChanged, this, &ResourcesProxyModel::invalidateFilter);

    m_currentStream = ResourcesModel::global()->search(m_filters);
    beginResetModel();
    m_displayedResources.clear();
    endResetModel();

    connect(m_currentStream, &AggregatedResultsStream::resourcesFound, this, [this](const QVector<AbstractResource*>& resources) {
        addResources(resources);
    });
    connect(m_currentStream, &AggregatedResultsStream::destroyed, this, [this]() {
        m_currentStream = nullptr;
        Q_EMIT busyChanged(false);
    });
    Q_EMIT busyChanged(true);
}

int ResourcesProxyModel::rowCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : m_displayedResources.count();
}

bool ResourcesProxyModel::lessThan(AbstractResource* leftPackage, AbstractResource* rightPackage) const
{
    if (m_sortByRelevancy) {
        //TODO: look into how to merge different sources
        Q_FOREACH (AbstractResource* res, m_displayedResources) {
            if(res == leftPackage)
                return true;
            else if(res == rightPackage)
                return false;
        }
        Q_UNREACHABLE();
    }

    auto role = m_sortRole;
    Qt::SortOrder order = m_sortOrder;
    QVariant leftValue;
    QVariant rightValue;
    //if we're comparing two equal values, we want the model sorted by application name
    if(role != ResourcesModel::NameRole) {
        leftValue = roleToValue(leftPackage, role);
        rightValue = roleToValue(rightPackage, role);

        if (leftValue == rightValue) {
            role = ResourcesModel::NameRole;
            order = Qt::DescendingOrder;
        }
    }

    bool ret;
    if(role == ResourcesModel::NameRole) {
        ret = leftPackage->nameSortKey().compare(rightPackage->nameSortKey()) < 0;
    } else if(role == ResourcesModel::CanUpgrade) {
        ret = leftValue.toBool();
    } else {
        ret = leftValue < rightValue;
    }
    return ret != (order == Qt::AscendingOrder);
}

Category* ResourcesProxyModel::filteredCategory() const
{
    return m_filters.category;
}

void ResourcesProxyModel::setSortByRelevancy(bool sort)
{
    if (sort != m_sortByRelevancy) {
        m_sortByRelevancy = sort;
        invalidateSorting();
    }
}

bool ResourcesProxyModel::sortingByRelevancy() const
{
    return m_sortByRelevancy;
}

void ResourcesProxyModel::setStateFilter(AbstractResource::State s)
{
    if (s != m_filters.state) {
        m_filters.state = s;
        invalidateFilter();
        emit stateFilterChanged();
    }
}

AbstractResource::State ResourcesProxyModel::stateFilter() const
{
    return m_filters.state;
}

QString ResourcesProxyModel::mimeTypeFilter() const
{
    return m_filters.mimetype;
}

void ResourcesProxyModel::setMimeTypeFilter(const QString& mime)
{
    if (m_filters.mimetype != mime) {
        m_filters.mimetype = mime;
        invalidateFilter();
    }
}

QString ResourcesProxyModel::extends() const
{
    return m_filters.extends;
}

void ResourcesProxyModel::setExtends(const QString& extends)
{
    if (m_filters.extends != extends) {
        m_filters.extends = extends;
        invalidateFilter();
    }
}

QVariant ResourcesProxyModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid()) {
        return QVariant();
    }

    AbstractResource* const resource = m_displayedResources[index.row()];
    return roleToValue(resource, role);
}

QVariant ResourcesProxyModel::roleToValue(AbstractResource* resource, int role) const
{
    switch(role) {
        case ResourcesModel::ActiveRole:
            return TransactionModel::global()->transactionFromResource(resource) != nullptr;
        case ResourcesModel::ApplicationRole:
            return qVariantFromValue<QObject*>(resource);
        case ResourcesModel::RatingPointsRole:
        case ResourcesModel::RatingRole:
        case ResourcesModel::RatingCountRole:
        case ResourcesModel::SortableRatingRole: {
            Rating* const rating = resource->rating();
            const int idx = Rating::staticMetaObject.indexOfProperty(roleNames().value(role).constData());
            Q_ASSERT(idx >= 0);
            auto prop = Rating::staticMetaObject.property(idx);
            if (rating)
                return prop.read(rating);
            else {
                QVariant val(0);
                val.convert(prop.type());
                return val;
            }
        }
        case Qt::DecorationRole:
        case Qt::DisplayRole:
        case Qt::StatusTipRole:
        case Qt::ToolTipRole:
            return QVariant();
        default: {
            QByteArray roleText = roleNames().value(role);
            if(Q_UNLIKELY(roleText.isEmpty())) {
                qDebug() << "unsupported role" << role;
                return {};
            }
            static const QMetaObject* m = &AbstractResource::staticMetaObject;
            int propidx = roleText.isEmpty() ? -1 : m->indexOfProperty(roleText.constData());

            if(Q_UNLIKELY(propidx < 0)) {
                qWarning() << "unknown role:" << role << roleText;
                return QVariant();
            } else
                return m->property(propidx).read(resource);
        }
    }
}

void ResourcesProxyModel::refreshResource(AbstractResource* resource, const QVector<QByteArray>& properties)
{
    const auto residx = m_displayedResources.indexOf(resource);
    auto include = m_filters.shouldFilter(resource);
    if (residx<0) {
        if (include) {
            const auto finder = [this, resource](AbstractResource* res){ return lessThan(res, resource); };
            const auto it = std::find_if(m_displayedResources.constBegin(), m_displayedResources.constEnd(), finder);
            auto newIdx = it == m_displayedResources.constEnd() ? m_displayedResources.count() : (it - m_displayedResources.constBegin());

            beginInsertRows({}, newIdx, newIdx);
            m_displayedResources.insert(newIdx, resource);
            endInsertRows();
        }
        return;
    }

    if (!m_filters.shouldFilter(resource)) {
        beginRemoveRows({}, residx, residx);
        m_displayedResources.removeAt(residx);
        endRemoveRows();
        return;
    }

    const QModelIndex idx = index(residx, 0);
    Q_ASSERT(idx.isValid());
    const auto roles = propertiesToRoles(properties);
    if (roles.contains(m_sortRole))
        invalidateSorting();
    else
        emit dataChanged(idx, idx, roles);
}

void ResourcesProxyModel::removeResource(AbstractResource* resource)
{
    const auto residx = m_displayedResources.indexOf(resource);
    if (residx < 0)
        return;
    beginRemoveRows({}, residx, residx);
    m_displayedResources.removeAt(residx);
    endRemoveRows();
}

void ResourcesProxyModel::refreshBackend(AbstractResourcesBackend* backend, const QVector<QByteArray>& properties)
{
    auto roles = propertiesToRoles(properties);
    const int count = m_displayedResources.count();

    bool found = false;

    for(int i = 0; i<count; ++i) {
        if (backend != m_displayedResources[i]->backend())
            continue;

        int j = i+1;
        for(; j<count && backend == m_displayedResources[j]->backend(); ++j)
        {}

        Q_EMIT dataChanged(index(i, 0), index(j-1, 0), roles);
        i = j;
        found = true;
    }

    if (found && properties.contains(m_roles.value(m_sortRole))) {
        invalidateSorting();
    }
}

void ResourcesProxyModel::resourceChangedByTransaction(Transaction* t)
{
    if (!t->resource())
        return;

    Q_ASSERT(!t->resource()->backend()->isFetching());
    const QModelIndex idx = index(m_displayedResources.indexOf(t->resource()), 0);
    if(idx.isValid())
        emit dataChanged(idx, idx, {
            ResourcesModel::StateRole, ResourcesModel::ActiveRole,
            ResourcesModel::InstalledRole, ResourcesModel::CanUpgrade,
            ResourcesModel::SizeRole
        });
}

QVector<int> ResourcesProxyModel::propertiesToRoles(const QVector<QByteArray>& properties) const
{
    QVector<int> roles = kTransform<QVector<int>>(properties, [this](const QByteArray& arr) { return roleNames().key(arr, -1); });
    roles.removeAll(-1);
    return roles;
}
