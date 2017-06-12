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
    , m_sortRole(NameRole)
    , m_sortOrder(Qt::AscendingOrder)
    , m_sortByRelevancy(false)
    , m_roles({
        { NameRole, "name" },
        { IconRole, "icon" },
        { CommentRole, "comment" },
        { StateRole, "state" },
        { RatingRole, "rating" },
        { RatingPointsRole, "ratingPoints" },
        { RatingCountRole, "ratingCount" },
        { SortableRatingRole, "sortableRating" },
        { InstalledRole, "isInstalled" },
        { ApplicationRole, "application" },
        { OriginRole, "origin" },
        { DisplayOriginRole, "displayOrigin" },
        { CanUpgrade, "canUpgrade" },
        { PackageNameRole, "packageName" },
        { IsTechnicalRole, "isTechnical" },
        { CategoryRole, "category" },
        { CategoryDisplayRole, "categoryDisplay" },
        { SectionRole, "section" },
        { MimeTypes, "mimetypes" },
        { LongDescriptionRole, "longDescription" },
        { SizeRole, "size" }
        })
    , m_currentStream(nullptr)
{
//     new ModelTest(this, this);

    connect(ResourcesModel::global(), &ResourcesModel::backendsChanged, this, &ResourcesProxyModel::invalidateFilter);
    connect(ResourcesModel::global(), &ResourcesModel::backendDataChanged, this, &ResourcesProxyModel::refreshBackend);
    connect(ResourcesModel::global(), &ResourcesModel::resourceDataChanged, this, &ResourcesProxyModel::refreshResource);
    connect(ResourcesModel::global(), &ResourcesModel::resourceRemoved, this, &ResourcesProxyModel::removeResource);
}

void ResourcesProxyModel::componentComplete()
{
    m_setup = true;
    invalidateFilter();
}

QHash<int, QByteArray> ResourcesProxyModel::roleNames() const
{
    return m_roles;
}

void ResourcesProxyModel::setSortRole(Roles sortRole)
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
        m_sortOrder = sortOrder;
        Q_EMIT sortRoleChanged(sortOrder);
        invalidateSorting();
    }
}

void ResourcesProxyModel::setSearch(const QString &_searchText)
{
    // 1-character searches are painfully slow. >= 2 chars are fine, though
    const QString searchText = _searchText.count() <= 1 ? QString() : _searchText;

    const bool diff = searchText != m_filters.search;

    if (diff) {
        m_filters.search = searchText;
        m_sortByRelevancy = !searchText.isEmpty();
        invalidateFilter();
        Q_EMIT searchChanged(m_filters.search);
    }
}

void ResourcesProxyModel::addResources(const QVector<AbstractResource *>& _res)
{
    auto res = _res;
    m_filters.filterJustInCase(res);

    if (res.isEmpty())
        return;

    if (!m_sortByRelevancy)
        qSort(res.begin(), res.end(), [this](AbstractResource* res, AbstractResource* res2){ return lessThan(res, res2); });

    sortedInsertion(res);
    fetchSubcategories();
}

void ResourcesProxyModel::invalidateSorting()
{
    if (m_displayedResources.isEmpty())
        return;

    if (!m_sortByRelevancy) {
        beginResetModel();
        qSort(m_displayedResources.begin(), m_displayedResources.end(), [this](AbstractResource* res, AbstractResource* res2){ return lessThan(res, res2); });
        endResetModel();
    }
}

QString ResourcesProxyModel::lastSearch() const
{
    return m_filters.search;
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
}

void ResourcesProxyModel::fetchSubcategories()
{
    const auto cats = m_filters.category ? m_filters.category->subCategories() : CategoryModel::global()->rootCategories();

    const int count = rowCount();
    QSet<Category*> done;
    for (int i=0; i<count; ++i) {
        AbstractResource* res = m_displayedResources[i];
        done.unite(res->categoryObjects());
    }

    const QVariantList ret = kTransform<QVariantList>(done, [](Category* cat) { return QVariant::fromValue<QObject*>(cat); });
    if (ret != m_subcategories) {
        m_subcategories = ret;
        Q_EMIT subcategoriesChanged(m_subcategories);
    }
}

QVariantList ResourcesProxyModel::subcategories() const
{
    return m_subcategories;
}

void ResourcesProxyModel::invalidateFilter()
{
    if (!m_setup || ResourcesModel::global()->backends().isEmpty()) {
        return;
    }

    if (m_currentStream) {
        qWarning() << "last stream isn't over yet" << m_filters << this;
        delete m_currentStream;
    }

    m_currentStream = ResourcesModel::global()->search(m_filters);
    beginResetModel();
    m_displayedResources.clear();
    endResetModel();

    connect(m_currentStream, &AggregatedResultsStream::resourcesFound, this, [this](const QVector<AbstractResource*>& resources) {
        addResources(resources);
    });
    connect(m_currentStream, &AggregatedResultsStream::finished, this, [this]() {
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
    auto role = m_sortRole;
    Qt::SortOrder order = m_sortOrder;
    QVariant leftValue;
    QVariant rightValue;
    //if we're comparing two equal values, we want the model sorted by application name
    if(role != NameRole) {
        leftValue = roleToValue(leftPackage, role);
        rightValue = roleToValue(rightPackage, role);

        if (leftValue == rightValue) {
            role = NameRole;
            order = Qt::DescendingOrder;
        }
    }

    bool ret;
    if(role == NameRole) {
        ret = leftPackage->nameSortKey().compare(rightPackage->nameSortKey()) < 0;
    } else if(role == CanUpgrade) {
        ret = leftValue.toBool();
    } else {
        ret = leftValue < rightValue;
    }
    return ret != (order != Qt::AscendingOrder);
}

Category* ResourcesProxyModel::filteredCategory() const
{
    return m_filters.category;
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

QUrl ResourcesProxyModel::resourcesUrl() const
{
    return m_filters.resourceUrl;
}

void ResourcesProxyModel::setResourcesUrl(const QUrl& resourcesUrl)
{
    if (m_filters.resourceUrl != resourcesUrl) {
        m_filters.resourceUrl = resourcesUrl;
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
        case ApplicationRole:
            return qVariantFromValue<QObject*>(resource);
        case RatingPointsRole:
        case RatingRole:
        case RatingCountRole:
        case SortableRatingRole: {
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

void ResourcesProxyModel::sortedInsertion(const QVector<AbstractResource*> & resources)
{
    Q_ASSERT(!resources.isEmpty());
    if (m_sortByRelevancy) {
        int rows = rowCount();
        beginInsertRows({}, rows, rows+resources.count()-1);
        m_displayedResources += resources;
        endInsertRows();
        return;
    }

    int newIdx = 0;
    for(auto resource: resources) {
        const auto finder = [this, resource](AbstractResource* res){ return lessThan(resource, res); };
        const auto it = std::find_if(m_displayedResources.constBegin() + newIdx, m_displayedResources.constEnd(), finder);
        newIdx = it == m_displayedResources.constEnd() ? m_displayedResources.count() : (it - m_displayedResources.constBegin());

        beginInsertRows({}, newIdx, newIdx);
        m_displayedResources.insert(newIdx, resource);
        endInsertRows();
    }
}

void ResourcesProxyModel::refreshResource(AbstractResource* resource, const QVector<QByteArray>& properties)
{
    const auto residx = m_displayedResources.indexOf(resource);
    if (residx<0) {
        if (!m_sortByRelevancy && m_filters.shouldFilter(resource)) {
            sortedInsertion({resource});
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
    if (roles.contains(m_sortRole)) {
        beginRemoveRows({}, residx, residx);
        m_displayedResources.removeAt(residx);
        endRemoveRows();

        sortedInsertion({resource});
    } else
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

QVector<int> ResourcesProxyModel::propertiesToRoles(const QVector<QByteArray>& properties) const
{
    QVector<int> roles = kTransform<QVector<int>>(properties, [this](const QByteArray& arr) { return roleNames().key(arr, -1); });
    roles.removeAll(-1);
    return roles;
}

int ResourcesProxyModel::indexOf(AbstractResource* res)
{
    return m_displayedResources.indexOf(res);
}

AbstractResource * ResourcesProxyModel::resourceAt(int row) const
{
    return m_displayedResources[row];
}
