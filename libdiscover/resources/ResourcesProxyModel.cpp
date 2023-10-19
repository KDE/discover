/*
 *   SPDX-FileCopyrightText: 2010 Jonathan Thomas <echidnaman@kubuntu.org>
 *   SPDX-FileCopyrightText: 2012 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "ResourcesProxyModel.h"

#include "libdiscover_debug.h"
#include <QMetaProperty>
#include <cmath>
#include <utils.h>

#include "ResourcesModel.h"
#include <Category/CategoryModel.h>
#include <KLocalizedString>
#include <ReviewsBackend/Rating.h>
#include <Transaction/TransactionModel.h>

const QHash<int, QByteArray> ResourcesProxyModel::s_roles = {{NameRole, "name"},
                                                             {IconRole, "icon"},
                                                             {CommentRole, "comment"},
                                                             {StateRole, "state"},
                                                             {RatingRole, "rating"},
                                                             {RatingPointsRole, "ratingPoints"},
                                                             {RatingCountRole, "ratingCount"},
                                                             {SortableRatingRole, "sortableRating"},
                                                             {InstalledRole, "isInstalled"},
                                                             {ApplicationRole, "application"},
                                                             {OriginRole, "origin"},
                                                             {DisplayOriginRole, "displayOrigin"},
                                                             {CanUpgrade, "canUpgrade"},
                                                             {PackageNameRole, "packageName"},
                                                             {CategoryRole, "category"},
                                                             {SectionRole, "section"},
                                                             {MimeTypes, "mimetypes"},
                                                             {LongDescriptionRole, "longDescription"},
                                                             {SourceIconRole, "sourceIcon"},
                                                             {SizeRole, "size"},
                                                             {ReleaseDateRole, "releaseDate"}};

ResourcesProxyModel::ResourcesProxyModel(QObject *parent)
    : QAbstractListModel(parent)
    , m_sortRole(NameRole)
    , m_sortOrder(Qt::AscendingOrder)
    , m_sortByRelevancy(false)
    , m_currentStream(nullptr)
{
    // new QAbstractItemModelTester(this, this);

    connect(ResourcesModel::global(), &ResourcesModel::backendsChanged, this, &ResourcesProxyModel::invalidateFilter);
    connect(ResourcesModel::global(), &ResourcesModel::backendDataChanged, this, &ResourcesProxyModel::refreshBackend);
    connect(ResourcesModel::global(), &ResourcesModel::resourceDataChanged, this, &ResourcesProxyModel::refreshResource);
    connect(ResourcesModel::global(), &ResourcesModel::resourceRemoved, this, &ResourcesProxyModel::removeResource);

    connect(this, &QAbstractItemModel::modelReset, this, &ResourcesProxyModel::countChanged);
    connect(this, &QAbstractItemModel::rowsInserted, this, &ResourcesProxyModel::countChanged);
    connect(this, &QAbstractItemModel::rowsRemoved, this, &ResourcesProxyModel::countChanged);

    QTimer *roughCountTimer = new QTimer(this);
    roughCountTimer->setInterval(10);
    roughCountTimer->setSingleShot(true);

    connect(roughCountTimer, &QTimer::timeout, this, &ResourcesProxyModel::roughCountChanged);
    connect(this, &ResourcesProxyModel::countChanged, roughCountTimer, qOverload<>(&QTimer::start));
    connect(this, &ResourcesProxyModel::busyChanged, roughCountTimer, qOverload<>(&QTimer::start));
}

void ResourcesProxyModel::componentComplete()
{
    m_setup = true;
    invalidateFilter();
}

QHash<int, QByteArray> ResourcesProxyModel::roleNames() const
{
    return s_roles;
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
        Q_EMIT sortOrderChanged(sortOrder);
        invalidateSorting();
    }
}

void ResourcesProxyModel::setSearch(const QString &_searchText)
{
    // 1-character searches are painfully slow. >= 2 chars are fine, though
    const QString searchText = _searchText.size() <= 1 ? QString() : _searchText;

    const bool diff = searchText != m_filters.search;

    if (diff) {
        m_filters.search = searchText;
        if (m_sortByRelevancy == searchText.isEmpty()) {
            m_sortByRelevancy = !searchText.isEmpty();
            Q_EMIT sortByRelevancyChanged(m_sortByRelevancy);
        }
        invalidateFilter();
        Q_EMIT searchChanged(m_filters.search);
    }
}

void ResourcesProxyModel::removeDuplicates(QList<StreamResult> &resources)
{
    const auto cab = ResourcesModel::global()->currentApplicationBackend();
    QHash<QString, QString> aliases;
    QHash<QString, QList<StreamResult>::iterator> storedIds;
    for (auto it = m_displayedResources.begin(); it != m_displayedResources.end(); ++it) {
        const auto appstreamid = it->resource->appstreamId();
        if (appstreamid.isEmpty()) {
            continue;
        }
        auto at = storedIds.find(appstreamid);
        if (at == storedIds.end()) {
            storedIds[appstreamid] = it;
        } else {
            qCWarning(LIBDISCOVER_LOG) << "We should have sanitized the displayed resources. There is a bug";
            Q_UNREACHABLE();
        }

        const auto alts = it->resource->alternativeAppstreamIds();
        for (const auto &alias : alts) {
            aliases[alias] = appstreamid;
        }
    }

    QHash<QString, QList<StreamResult>::iterator> ids;
    for (auto it = resources.begin(); it != resources.end();) {
        const auto appstreamid = it->resource->appstreamId();
        if (appstreamid.isEmpty()) {
            ++it;
            continue;
        }
        auto at = storedIds.find(appstreamid);
        if (at == storedIds.end()) {
            auto aliased = aliases.constFind(appstreamid);
            if (aliased != aliases.constEnd()) {
                at = storedIds.find(aliased.value());
            }
        }

        if (at == storedIds.end()) {
            const auto alts = it->resource->alternativeAppstreamIds();
            for (const auto &alt : alts) {
                at = storedIds.find(alt);
                if (at == storedIds.end())
                    break;

                auto aliased = aliases.constFind(alt);
                if (aliased != aliases.constEnd()) {
                    at = storedIds.find(aliased.value());
                    if (at != storedIds.end())
                        break;
                }
            }
        }
        if (at == storedIds.end()) {
            auto at = ids.find(appstreamid);
            if (at == ids.end()) {
                auto aliased = aliases.constFind(appstreamid);
                if (aliased != aliases.constEnd()) {
                    at = ids.find(aliased.value());
                }
            }
            if (at == ids.end()) {
                const auto alts = it->resource->alternativeAppstreamIds();
                for (const auto &alt : alts) {
                    at = ids.find(alt);
                    if (at != ids.end())
                        break;

                    auto aliased = aliases.constFind(appstreamid);
                    if (aliased != aliases.constEnd()) {
                        at = ids.find(aliased.value());
                        if (at != ids.end())
                            break;
                    }
                }
            }
            if (at == ids.end()) {
                ids[appstreamid] = it;
                const auto alts = it->resource->alternativeAppstreamIds();
                for (const auto &alias : alts) {
                    aliases[alias] = appstreamid;
                }
                ++it;
            } else {
                if (it->resource->backend() == cab && it->resource->backend() != (**at).resource->backend()) {
                    qSwap(*it, **at);
                }
                it = resources.erase(it);
            }
        } else {
            if (it->resource->backend() == cab) {
                **at = *it;
                auto pos = index(*at - m_displayedResources.begin(), 0);
                Q_EMIT dataChanged(pos, pos);
            }
            it = resources.erase(it);
        }
    }
}

void ResourcesProxyModel::addResources(const QList<StreamResult> &_res)
{
    auto res = _res;
    m_filters.filterJustInCase(res);

    if (res.isEmpty())
        return;

    if (!m_sortByRelevancy)
        std::sort(res.begin(), res.end(), [this](auto res, auto res2) {
            return orderedLessThan(res, res2);
        });

    sortedInsertion(res);
    fetchSubcategories();
}

void ResourcesProxyModel::invalidateSorting()
{
    if (m_displayedResources.isEmpty())
        return;

    if (!m_sortByRelevancy) {
        beginResetModel();
        std::sort(m_displayedResources.begin(), m_displayedResources.end(), [this](auto res, auto res2) {
            return orderedLessThan(res, res2);
        });
        endResetModel();
    }
}

QString ResourcesProxyModel::lastSearch() const
{
    return m_filters.search;
}

void ResourcesProxyModel::setOriginFilter(const QString &origin)
{
    if (origin == m_filters.origin)
        return;

    m_filters.origin = origin;

    invalidateFilter();
}

QString ResourcesProxyModel::originFilter() const
{
    return m_filters.origin;
}

QString ResourcesProxyModel::filteredCategoryName() const
{
    return m_categoryName;
}

void ResourcesProxyModel::setFilteredCategoryName(const QString &cat)
{
    if (cat == m_categoryName)
        return;

    m_categoryName = cat;

    auto category = CategoryModel::global()->findCategoryByName(cat);
    if (category) {
        setFiltersFromCategory(category);
    } else {
        qDebug() << "looking up wrong category or too early" << m_categoryName;
        auto f = [this, cat] {
            auto category = CategoryModel::global()->findCategoryByName(cat);
            setFiltersFromCategory(category);
        };
        auto one = new OneTimeAction(f, this);
        connect(CategoryModel::global(), &CategoryModel::rootCategoriesChanged, one, &OneTimeAction::trigger);
    }
}

void ResourcesProxyModel::setFiltersFromCategory(Category *category)
{
    if (category == m_filters.category)
        return;

    m_filters.category = category;
    invalidateFilter();
    Q_EMIT categoryChanged();
}

void ResourcesProxyModel::fetchSubcategories()
{
    auto cats = kToSet(m_filters.category ? m_filters.category->subCategories() : CategoryModel::global()->rootCategories());

    const int count = rowCount();
    QSet<Category *> done;
    for (int i = 0; i < count && !cats.isEmpty(); ++i) {
        AbstractResource *res = m_displayedResources[i].resource;
        const auto found = res->categoryObjects(kSetToVector(cats));
        done.unite(found);
        cats.subtract(found);
    }

    const QVariantList ret = kTransform<QVariantList>(done, [](Category *cat) {
        return QVariant::fromValue<QObject *>(cat);
    });
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

    if (!m_categoryName.isEmpty() && m_filters.category == nullptr) {
        return;
    }

    if (m_currentStream) {
        qCWarning(LIBDISCOVER_LOG) << "last stream isn't over yet" << m_filters << this;
        delete m_currentStream;
    }

    m_currentStream = m_filters.backend ? m_filters.backend->search(m_filters) : ResourcesModel::global()->search(m_filters);
    Q_EMIT busyChanged(true);

    if (!m_displayedResources.isEmpty()) {
        beginResetModel();
        m_displayedResources.clear();
        endResetModel();
    }

    connect(m_currentStream, &ResultsStream::resourcesFound, this, &ResourcesProxyModel::addResources);
    connect(m_currentStream, &ResultsStream::destroyed, this, [this]() {
        m_currentStream = nullptr;
        Q_EMIT busyChanged(false);
    });
}

int ResourcesProxyModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_displayedResources.count();
}

bool ResourcesProxyModel::lessThan(const StreamResult &left, const StreamResult &right) const
{
    if (left.sortScore == right.sortScore) {
        return lessThan(left.resource, right.resource);
    }
    return left.sortScore < right.sortScore;
}

bool ResourcesProxyModel::orderedLessThan(const StreamResult &left, const StreamResult &right) const
{
    bool less = lessThan(left, right);
    return m_sortOrder == Qt::AscendingOrder ? less : !less;
}

bool ResourcesProxyModel::lessThan(AbstractResource *leftPackage, AbstractResource *rightPackage) const
{
    auto role = m_sortRole;
    Qt::SortOrder order = m_sortOrder;
    QVariant leftValue;
    QVariant rightValue;
    // if we're comparing two equal values, we want the model sorted by application name
    if (role != NameRole) {
        leftValue = roleToValue(leftPackage, role);
        rightValue = roleToValue(rightPackage, role);

        if (leftValue == rightValue) {
            role = NameRole;
            order = Qt::AscendingOrder;
        }
    }

    bool ret;
    if (role == NameRole) {
        ret = leftPackage->nameSortKey().compare(rightPackage->nameSortKey()) < 0;
    } else if (role == CanUpgrade) {
        ret = leftValue.toBool();
    } else {
        const auto order = QVariant::compare(leftValue, rightValue);
        Q_ASSERT(order != QPartialOrdering::Unordered);
        return order == QPartialOrdering::Less;
    }
    return ret != (order != Qt::AscendingOrder);
}

Category *ResourcesProxyModel::filteredCategory() const
{
    return m_filters.category;
}

void ResourcesProxyModel::setStateFilter(AbstractResource::State s)
{
    if (s != m_filters.state) {
        m_filters.state = s;
        invalidateFilter();
        Q_EMIT stateFilterChanged();
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

void ResourcesProxyModel::setMimeTypeFilter(const QString &mime)
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

void ResourcesProxyModel::setExtends(const QString &extends)
{
    if (m_filters.extends != extends) {
        m_filters.extends = extends;
        invalidateFilter();
    }
}

void ResourcesProxyModel::setFilterMinimumState(bool filterMinimumState)
{
    if (filterMinimumState != m_filters.filterMinimumState) {
        m_filters.filterMinimumState = filterMinimumState;
        invalidateFilter();
        Q_EMIT filterMinimumStateChanged(m_filters.filterMinimumState);
    }
}

bool ResourcesProxyModel::filterMinimumState() const
{
    return m_filters.filterMinimumState;
}

QUrl ResourcesProxyModel::resourcesUrl() const
{
    return m_filters.resourceUrl;
}

void ResourcesProxyModel::setResourcesUrl(const QUrl &resourcesUrl)
{
    if (m_filters.resourceUrl != resourcesUrl) {
        m_filters.resourceUrl = resourcesUrl;
        invalidateFilter();
    }
}

bool ResourcesProxyModel::allBackends() const
{
    return m_filters.allBackends;
}

void ResourcesProxyModel::setAllBackends(bool allBackends)
{
    m_filters.allBackends = allBackends;
}

AbstractResourcesBackend *ResourcesProxyModel::backendFilter() const
{
    return m_filters.backend;
}

void ResourcesProxyModel::setBackendFilter(AbstractResourcesBackend *filtered)
{
    m_filters.backend = filtered;
}

QVariant ResourcesProxyModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) {
        return QVariant();
    }

    const auto result = m_displayedResources[index.row()];
    return roleToValue(result.resource, role);
}

QVariant ResourcesProxyModel::roleToValue(AbstractResource *resource, int role) const
{
    switch (role) {
    case ApplicationRole:
        return QVariant::fromValue<QObject *>(resource);
    case RatingPointsRole:
    case RatingRole:
    case RatingCountRole:
    case SortableRatingRole: {
        Rating *const rating = resource->rating();
        const int idx = Rating::staticMetaObject.indexOfProperty(roleNames().value(role).constData());
        Q_ASSERT(idx >= 0);
        auto prop = Rating::staticMetaObject.property(idx);
        if (rating) {
            return prop.readOnGadget(rating);
        } else {
            QVariant val(0);
            val.convert(prop.metaType());
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
        if (Q_UNLIKELY(roleText.isEmpty())) {
            qCDebug(LIBDISCOVER_LOG) << "unsupported role" << role;
            return {};
        }
        static const QMetaObject *m = &AbstractResource::staticMetaObject;
        int propidx = roleText.isEmpty() ? -1 : m->indexOfProperty(roleText.constData());

        if (Q_UNLIKELY(propidx < 0)) {
            qCWarning(LIBDISCOVER_LOG) << "unknown role:" << role << roleText;
            return QVariant();
        } else
            return m->property(propidx).read(resource);
    }
    }
}

bool ResourcesProxyModel::isSorted(const QList<StreamResult> &resources)
{
    auto last = resources.constFirst();
    for (auto it = resources.constBegin() + 1, itEnd = resources.constEnd(); it != itEnd; ++it) {
        auto v1 = roleToValue(last, m_sortRole), v2 = roleToValue(*it, m_sortRole);
        if (!lessThan(last, *it) && v1 != v2) {
            qDebug() << "faulty sort" << last.resource->name() << (*it).resource->name() << last.resource << (*it).resource;
            return false;
        }
        last = *it;
    }
    return true;
}

void ResourcesProxyModel::sortedInsertion(const QList<StreamResult> &_res)
{
    Q_ASSERT(_res.size() == QSet(_res.constBegin(), _res.constEnd()).size());

    auto resources = _res;
    Q_ASSERT(!resources.isEmpty());

    if (!m_filters.allBackends) {
        removeDuplicates(resources);
        if (resources.isEmpty())
            return;
    }

    if (m_sortByRelevancy || m_displayedResources.isEmpty()) {
        // Q_ASSERT(m_sortByRelevancy || isSorted(resources));
        int rows = rowCount();
        beginInsertRows({}, rows, rows + resources.count() - 1);
        m_displayedResources += resources;
        endInsertRows();
        return;
    }

    for (auto result : std::as_const(resources)) {
        const auto finder = [this](StreamResult result, StreamResult res) {
            return orderedLessThan(result, res);
        };
        const auto it = std::upper_bound(m_displayedResources.constBegin(), m_displayedResources.constEnd(), result, finder);
        const auto newIdx = it == m_displayedResources.constEnd() ? m_displayedResources.count() : (it - m_displayedResources.constBegin());

        if ((it - 1) != m_displayedResources.constEnd() && (it - 1)->resource == result.resource)
            continue;

        beginInsertRows({}, newIdx, newIdx);
        m_displayedResources.insert(newIdx, result);
        endInsertRows();
        // Q_ASSERT(isSorted(resources));
    }
}

void ResourcesProxyModel::refreshResource(AbstractResource *resource, const QList<QByteArray> &properties)
{
    const auto residx = indexOf(resource);
    if (residx < 0) {
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
    if (!m_sortByRelevancy && roles.contains(m_sortRole)) {
        beginRemoveRows({}, residx, residx);
        m_displayedResources.removeAt(residx);
        endRemoveRows();

        sortedInsertion({{resource, 0}});
    } else
        Q_EMIT dataChanged(idx, idx, roles);
}

void ResourcesProxyModel::removeResource(AbstractResource *resource)
{
    const auto residx = indexOf(resource);
    if (residx < 0)
        return;
    beginRemoveRows({}, residx, residx);
    m_displayedResources.removeAt(residx);
    endRemoveRows();
}

void ResourcesProxyModel::refreshBackend(AbstractResourcesBackend *backend, const QList<QByteArray> &properties)
{
    auto roles = propertiesToRoles(properties);
    const int count = m_displayedResources.count();

    bool found = false;

    for (int i = 0; i < count; ++i) {
        if (backend != m_displayedResources[i].resource->backend())
            continue;

        int j = i + 1;
        for (; j < count && backend == m_displayedResources[j].resource->backend(); ++j) { }

        Q_EMIT dataChanged(index(i, 0), index(j - 1, 0), roles);
        i = j;
        found = true;
    }

    if (found && properties.contains(s_roles.value(m_sortRole))) {
        invalidateSorting();
    }
}

QList<int> ResourcesProxyModel::propertiesToRoles(const QList<QByteArray> &properties) const
{
    QList<int> roles = kTransform<QList<int>>(properties, [this](const QByteArray &arr) {
        return roleNames().key(arr, -1);
    });
    roles.removeAll(-1);
    return roles;
}

int ResourcesProxyModel::indexOf(AbstractResource *res)
{
    return kIndexOf(m_displayedResources, [res](auto result) {
        return result.resource == res;
    });
}

AbstractResource *ResourcesProxyModel::resourceAt(int row) const
{
    return m_displayedResources[row].resource;
}

bool ResourcesProxyModel::canFetchMore(const QModelIndex &parent) const
{
    Q_ASSERT(!parent.isValid());
    return m_currentStream;
}

void ResourcesProxyModel::fetchMore(const QModelIndex &parent)
{
    Q_ASSERT(!parent.isValid());
    if (!m_currentStream)
        return;
    Q_EMIT m_currentStream->fetchMore();
}

bool ResourcesProxyModel::sortByRelevancy() const
{
    return m_sortByRelevancy;
}

QString ResourcesProxyModel::roughCount() const
{
    const int rows = rowCount();
    if (isBusy()) {
        // We return an empty string because it's evidently confusing
        if (rows == 0) {
            return {};
        }

        // We convert rows=1234 into round=1000
        const int round = std::pow(10, std::floor(std::log10(rows)));
        if (round >= 1) {
            return i18nc("an approximation number, like 3000+", "%1+", (rows / round) * round);
        }
    }
    return QString::number(rows);
}
