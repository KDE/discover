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

#include "ResourcesModel.h"
#include <utils.h>

#include "AbstractResource.h"
#include "resources/AbstractResourcesBackend.h"
#include <ReviewsBackend/Rating.h>
#include <ReviewsBackend/AbstractReviewsBackend.h>
#include <Transaction/Transaction.h>
#include <DiscoverBackendsFactory.h>
#include <KActionCollection>
#include "Transaction/TransactionModel.h"
#include "Category/CategoryModel.h"
#include <QDebug>
#include <QCoreApplication>
#include <QThread>
#include <QAction>
#include <QMetaProperty>

ResourcesModel *ResourcesModel::s_self = nullptr;

ResourcesModel *ResourcesModel::global()
{
    if(!s_self)
        s_self = new ResourcesModel;
    return s_self;
}

ResourcesModel::ResourcesModel(QObject* parent, bool load)
    : QAbstractListModel(parent)
    , m_initializingBackends(0)
    , m_actionCollection(nullptr)
    , m_roles(QAbstractItemModel::roleNames().unite({
        { NameRole, "name" },
        { IconRole, "icon" },
        { CommentRole, "comment" },
        { StateRole, "state" },
        { RatingRole, "rating" },
        { RatingPointsRole, "ratingPoints" },
        { RatingCountRole, "ratingCount" },
        { SortableRatingRole, "sortableRating" },
        { ActiveRole, "active" },
        { InstalledRole, "isInstalled" },
        { ApplicationRole, "application" },
        { OriginRole, "origin" },
        { CanUpgrade, "canUpgrade" },
        { PackageNameRole, "packageName" },
        { IsTechnicalRole, "isTechnical" },
        { CategoryRole, "category" },
        { SectionRole, "section" },
        { MimeTypes, "mimetypes" },
        { LongDescriptionRole, "longDescription" },
        { SizeRole, "size" }
        })
    )
{
    init(load);
    connect(this, &ResourcesModel::allInitialized, this, &ResourcesModel::fetchingChanged);
}

void ResourcesModel::init(bool load)
{
    Q_ASSERT(!s_self);
    Q_ASSERT(QCoreApplication::instance()->thread()==QThread::currentThread());

    connect(TransactionModel::global(), &TransactionModel::transactionAdded, this, &ResourcesModel::resourceChangedByTransaction);
    connect(TransactionModel::global(), &TransactionModel::transactionRemoved, this, &ResourcesModel::resourceChangedByTransaction);
    if(load)
        QMetaObject::invokeMethod(this, "registerAllBackends", Qt::QueuedConnection);
}

ResourcesModel::ResourcesModel(const QString& backendName, QObject* parent)
    : ResourcesModel(parent, false)
{
    s_self = this;
    registerBackendByName(backendName);
}

ResourcesModel::~ResourcesModel()
{
    qDeleteAll(m_backends);
}

QHash<int, QByteArray> ResourcesModel::roleNames() const
{
    return m_roles;
}

void ResourcesModel::addResourcesBackend(AbstractResourcesBackend* backend)
{
    Q_ASSERT(!m_backends.contains(backend));
    if(!backend->isValid()) {
        qWarning() << "Discarding invalid backend" << backend->name();
        CategoryModel::blacklistPlugin(backend->name());
        backend->deleteLater();
        return;
    }

    if(!backend->isFetching()) {
        QVector<AbstractResource*> newResources = backend->allResources();
        int current = rowCount();
        beginInsertRows(QModelIndex(), current, current+newResources.size());
        m_backends += backend;
        m_resources.append(newResources);
        endInsertRows();
        emit updatesCountChanged();
    } else {
        m_initializingBackends++;
        m_backends += backend;
        m_resources.append(QVector<AbstractResource*>());
    }
    if(m_actionCollection)
        backend->integrateActions(m_actionCollection);

    connect(backend, &AbstractResourcesBackend::fetchingChanged, this, &ResourcesModel::callerFetchingChanged);
    connect(backend, &AbstractResourcesBackend::allDataChanged, this, &ResourcesModel::updateCaller);
    connect(backend, &AbstractResourcesBackend::resourcesChanged, this, &ResourcesModel::emitResourceChanges);
    connect(backend, &AbstractResourcesBackend::updatesCountChanged, this, &ResourcesModel::updatesCountChanged);
    connect(backend, &AbstractResourcesBackend::searchInvalidated, this, &ResourcesModel::searchInvalidated);

    emit backendsChanged();

    if(m_initializingBackends==0)
        emit allInitialized();
    else
        emit fetchingChanged();
}

AbstractResource* ResourcesModel::resourceAt(int row) const
{
    for (auto it = m_resources.constBegin(); it != m_resources.constEnd(); ++it) {
        if (it->size()<=row)
            row -= it->size();
        else
            return it->at(row);
    }
    return nullptr;
}

QModelIndex ResourcesModel::resourceIndex(AbstractResource* res) const
{
    AbstractResourcesBackend* backend = res->backend();
    int row = 0, backends = m_backends.count();
    for(int i=0; i<backends; i++) {
        if(m_backends[i]!=backend)
            row += m_resources[i].size();
        else {
            Q_ASSERT(!m_backends[i]->isFetching());
            int pos = m_resources[i].indexOf(res);
            return pos>=0 ? index(row+pos) : QModelIndex();
        }
    }

    return QModelIndex();
}

QVariant ResourcesModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid()) {
        return QVariant();
    }

    AbstractResource* const resource = resourceAt(index.row());
    switch(role) {
        case ActiveRole:
            return TransactionModel::global()->transactionFromResource(resource) != nullptr;
        case ApplicationRole:
            return qVariantFromValue<QObject*>(resource);
        case RatingPointsRole:
        case RatingRole:
        case RatingCountRole:
        case SortableRatingRole: {
            Rating* const rating = resource->rating();
            if (rating)
                return rating->property(roleNames().value(role).constData());
            else {
                const int idx = Rating::staticMetaObject.indexOfProperty(roleNames().value(role).constData());
                QVariant val = QVariant(0);
                val.convert(Rating::staticMetaObject.property(idx).type());
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
            const QMetaObject* m = resource->metaObject();
            int propidx = roleText.isEmpty() ? -1 : m->indexOfProperty(roleText.constData());

            if(Q_UNLIKELY(propidx < 0)) {
                qDebug() << "unknown role:" << role << roleText;
                return QVariant();
            } else
                return m->property(propidx).read(resource);
        }
    }
}

int ResourcesModel::rowCount(const QModelIndex& parent) const
{
    if(parent.isValid())
        return 0; // Not the root element, and children don't have subchildren

    // The root element parents all resources from all backends
    int ret = 0;
    Q_FOREACH (const QVector<AbstractResource*>& resources, m_resources)
        ret += resources.size();

    return ret;
}

int ResourcesModel::rowsBeforeBackend(AbstractResourcesBackend* backend, QVector<QVector<AbstractResource*>>::iterator& backendsResources)
{
    Q_ASSERT(backend);
    int pos = m_backends.indexOf(backend);
    Q_ASSERT(pos>=0);
    backendsResources = m_resources.begin()+pos;

    int before = 0;
    for(auto it = m_resources.constBegin();
        it != m_resources.constEnd() && it != backendsResources;
        ++it)
    {
        before+= it->size();
    }
    return before;
}

void ResourcesModel::cleanBackend(AbstractResourcesBackend* backend)
{
    QVector<QVector<AbstractResource*>>::iterator backendsResources;
    int before = rowsBeforeBackend(backend, backendsResources);

    if (backendsResources->isEmpty()) {
        return;
    }
    
    beginRemoveRows(QModelIndex(), before, before + backendsResources->count() - 1);
    backendsResources->clear();
    endRemoveRows();
}

void ResourcesModel::callerFetchingChanged()
{
    AbstractResourcesBackend* backend = qobject_cast<AbstractResourcesBackend*>(sender());

    if (!backend->isValid()) {
        qWarning() << "Discarding invalid backend" << backend->name();
        cleanBackend(backend);
        int idx = m_backends.indexOf(backend);
        Q_ASSERT(idx>=0);
        m_backends.removeAt(idx);
        m_resources.removeAt(idx);
        CategoryModel::blacklistPlugin(backend->name());
        backend->deleteLater();
        return;
    }

    if(backend->isFetching()) {
        m_initializingBackends++;
        cleanBackend(backend);
        emit fetchingChanged();
    } else {
        resetBackend(backend);

        m_initializingBackends--;
        if(m_initializingBackends==0)
            emit allInitialized();
        else
            emit fetchingChanged();
    }
}

void ResourcesModel::resetBackend(AbstractResourcesBackend* backend)
{
    QVector<AbstractResource*> res = backend->allResources();

    if(!res.isEmpty()) {
        QVector<QVector<AbstractResource*>>::iterator backendsResources;
        int before = rowsBeforeBackend(backend, backendsResources);
        Q_ASSERT(backendsResources->isEmpty());
        
        beginInsertRows(QModelIndex(), before, before+res.size()-1);
        *backendsResources = res;
        endInsertRows();
        emit updatesCountChanged();
    }
}

void ResourcesModel::updateCaller(const QVector<QByteArray>& properties)
{
    AbstractResourcesBackend* backend = qobject_cast<AbstractResourcesBackend*>(sender());
    QVector<QVector<AbstractResource*>>::iterator backendsResources;
    int before = rowsBeforeBackend(backend, backendsResources);
    if (backendsResources->isEmpty())
        return;
    
    emit dataChanged(index(before), index(before+backendsResources->size()-1), propertiesToRoles(properties));
}

QVector<int> ResourcesModel::propertiesToRoles(const QVector<QByteArray>& properties) const
{
    QVector<int> roles = kTransform<QVector<int>>(properties, [this](const QByteArray& arr) { return roleNames().key(arr, -1); });
    roles.removeAll(-1);
    return roles;
}

void ResourcesModel::emitResourceChanges(AbstractResource* resource, const QVector<QByteArray> &properties)
{
    const QModelIndex idx = resourceIndex(resource);
    if (!idx.isValid())
        return;

    emit dataChanged(idx, idx, propertiesToRoles(properties));
}

QVector< AbstractResourcesBackend* > ResourcesModel::backends() const
{
    return m_backends;
}

AbstractResource* ResourcesModel::resourceByPackageName(const QString& name)
{
    if (m_backends.isEmpty()) {
        qWarning() << "looking for package" << name << "without any backends loaded";
    }
    foreach(AbstractResourcesBackend* backend, m_backends)
    {
        AbstractResource* res = backend->resourceByPackageName(name);
        if(res) {
            return res;
        }
    }
    return nullptr;
}

int ResourcesModel::updatesCount() const
{
    int ret = 0;

    foreach(AbstractResourcesBackend* backend, m_backends) {
        ret += backend->updatesCount();
    }

    return ret;
}

void ResourcesModel::installApplication(AbstractResource* app)
{
    Q_ASSERT(!isFetching());
    app->backend()->installApplication(app);
}

void ResourcesModel::installApplication(AbstractResource* app, const AddonList& addons)
{
    Q_ASSERT(!isFetching());
    app->backend()->installApplication(app, addons);
}

void ResourcesModel::removeApplication(AbstractResource* app)
{
    Q_ASSERT(!isFetching());
    app->backend()->removeApplication(app);
}

QMap<int, QVariant> ResourcesModel::itemData(const QModelIndex& index) const
{
    QMap<int, QVariant> ret;
    QHash<int, QByteArray> names = roleNames();
    for (auto it = names.constBegin(); it != names.constEnd(); ++it) {
        ret.insert(it.key(), data(index, it.key()));
    }
    return ret;
}

void ResourcesModel::registerAllBackends()
{
    DiscoverBackendsFactory f;
    QList<AbstractResourcesBackend*> backends = f.allBackends();
    if(m_initializingBackends==0 && backends.isEmpty()) {
        qWarning() << "Couldn't find any backends";
        emit allInitialized();
    } else {
        foreach(AbstractResourcesBackend* b, backends) {
            addResourcesBackend(b);
        }
    }
}

void ResourcesModel::registerBackendByName(const QString& name)
{
    DiscoverBackendsFactory f;
    addResourcesBackend(f.backend(name));
}

void ResourcesModel::integrateActions(KActionCollection* w)
{
    Q_ASSERT(w->thread()==thread());
    m_actionCollection = w;
    setParent(w);
    foreach(AbstractResourcesBackend* b, m_backends) {
        b->integrateActions(w);
    }
}

void ResourcesModel::resourceChangedByTransaction(Transaction* t)
{
    if (!t->resource())
        return;

    Q_ASSERT(!t->resource()->backend()->isFetching());
    QModelIndex idx = resourceIndex(t->resource());
    if(idx.isValid())
        emit dataChanged(idx, idx);
}

bool ResourcesModel::isFetching() const
{
    foreach(AbstractResourcesBackend* b, m_backends) {
        if(b->isFetching())
            return true;
    }
    return false;
}

QList<QAction*> ResourcesModel::messageActions() const
{
    QList<QAction*> ret;
    foreach(AbstractResourcesBackend* b, m_backends) {
        ret += b->messageActions();
    }
    Q_ASSERT(!ret.contains(nullptr));
    return ret;
}

bool ResourcesModel::isBusy() const
{
    return TransactionModel::global()->rowCount() > 0;
}

bool ResourcesModel::isExtended(const QString& id)
{
    auto resourceExtends = [id](AbstractResource* res) {return res->extends().contains(id); };
    bool ret = false;
    foreach (const QVector<AbstractResource*> & backend, m_resources) {
        ret = std::find_if(backend.cbegin(), backend.cend(), resourceExtends) != backend.cend();
        if (ret)
            break;
    }
    return ret;
}
