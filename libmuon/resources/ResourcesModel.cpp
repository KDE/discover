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

#include "AbstractResource.h"
#include "resources/AbstractResourcesBackend.h"
#include <ReviewsBackend/Rating.h>
#include <ReviewsBackend/AbstractReviewsBackend.h>
#include <Transaction/Transaction.h>
#include <MuonBackendsFactory.h>
#include <MuonMainWindow.h>
#include "Transaction/TransactionModel.h"
#include <QDebug>
#include <QCoreApplication>
#include <QThread>

// static const KCatalogLoader loader("libmuon");//FIXME port

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
    , m_mainwindow(0)
{
    init(load);
    connect(this, SIGNAL(allInitialized()), SIGNAL(fetchingChanged()));
}

void ResourcesModel::init(bool load)
{
    Q_ASSERT(!s_self);
    Q_ASSERT(QCoreApplication::instance()->thread()==QThread::currentThread());

    connect(TransactionModel::global(), SIGNAL(transactionAdded(Transaction*)), SLOT(resourceChangedByTransaction(Transaction*)));
    connect(TransactionModel::global(), SIGNAL(transactionRemoved(Transaction*)), SLOT(resourceChangedByTransaction(Transaction*)));
    if(load)
        QMetaObject::invokeMethod(this, "registerAllBackends", Qt::QueuedConnection);
}

ResourcesModel::ResourcesModel(const QString& backendName, QObject* parent)
    : QAbstractListModel(parent)
    , m_initializingBackends(0)
    , m_mainwindow(0)
{
    init(false);
    Q_ASSERT(!s_self);
    s_self = this;
    registerBackendByName(backendName);
}

ResourcesModel::~ResourcesModel()
{
    qDeleteAll(m_backends);
}

QHash<int, QByteArray> ResourcesModel::roleNames() const
{
    QHash<int, QByteArray> roles = QAbstractItemModel::roleNames();
    roles[NameRole] = "name";
    roles[IconRole] = "icon";
    roles[CommentRole] = "comment";
    roles[StateRole] = "state";
    roles[RatingRole] = "rating";
    roles[RatingPointsRole] = "ratingPoints";
    roles[SortableRatingRole] = "sortableRating";
    roles[ActiveRole] = "active";
    roles[InstalledRole] = "isInstalled";
    roles[ApplicationRole] = "application";
    roles[OriginRole] = "origin";
    roles[CanUpgrade] = "canUpgrade";
    roles[PackageNameRole] = "packageName";
    roles[IsTechnicalRole] = "isTechnical";
    roles[CategoryRole] = "category";
    roles[SectionRole] = "section";
    roles[MimeTypes] = "mimetypes";
    roles.remove(Qt::EditRole);
    roles.remove(Qt::WhatsThisRole);
    return roles;
}

void ResourcesModel::addResourcesBackend(AbstractResourcesBackend* backend)
{
    Q_ASSERT(!m_backends.contains(backend));
    if(!backend->isFetching()) {
        QVector<AbstractResource*> newResources = backend->allResources();
        int current = rowCount();
        beginInsertRows(QModelIndex(), current, current+newResources.size());
        m_backends += backend;
        m_resources.append(newResources);
        endInsertRows();
    } else {
        m_initializingBackends++;
        m_backends += backend;
        m_resources.append(QVector<AbstractResource*>());
    }
    if(m_mainwindow)
        backend->integrateMainWindow(m_mainwindow);

    connect(backend, SIGNAL(fetchingChanged()), SLOT(callerFetchingChanged()));
    connect(backend, SIGNAL(allDataChanged()), SLOT(updateCaller()));
    connect(backend, SIGNAL(updatesCountChanged()), SIGNAL(updatesCountChanged()));
    connect(backend, SIGNAL(searchInvalidated()), SIGNAL(searchInvalidated()));

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
    return 0;
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
            Q_ASSERT(pos>=0);
            return index(row+pos);
        }
    }

    return QModelIndex();
}

QVariant ResourcesModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid()) {
        return QVariant();
    }

    AbstractResource* resource = resourceAt(index.row());
    switch(role) {
        case ActiveRole:
            return TransactionModel::global()->transactionFromResource(resource) != nullptr;
        case ApplicationRole:
            return qVariantFromValue<QObject*>(resource);
        case RatingPointsRole:
        case RatingRole:
        case SortableRatingRole: {
            Rating* rating = resource->rating();
            return rating ? rating->property(roleNames().value(role)) : -1;
        }
        case Qt::DecorationRole:
        case Qt::DisplayRole:
        case Qt::StatusTipRole:
        case Qt::ToolTipRole:
            return QVariant();
        default: {
            QByteArray roleText = roleNames().value(role);
            const QMetaObject* m = resource->metaObject();
            int propidx = roleText.isEmpty() ? -1 : m->indexOfProperty(roleText);

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
    for (const QVector<AbstractResource*>& resources : m_resources)
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
    m_initializingBackends++;
    
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
    if(backend->isFetching()) {
        cleanBackend(backend);
        emit fetchingChanged();
    } else {
        resetBackend(backend);
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
    m_initializingBackends--;
    if(m_initializingBackends==0)
        emit allInitialized();
}

void ResourcesModel::updateCaller()
{
    AbstractResourcesBackend* backend = qobject_cast<AbstractResourcesBackend*>(sender());
    QVector<QVector<AbstractResource*>>::iterator backendsResources;
    int before = rowsBeforeBackend(backend, backendsResources);
    if (backendsResources->isEmpty())
        return;
    
    emit dataChanged(index(before), index(before+backendsResources->size()-1));
}

QVector< AbstractResourcesBackend* > ResourcesModel::backends() const
{
    return m_backends;
}

AbstractResource* ResourcesModel::resourceByPackageName(const QString& name)
{
    foreach(AbstractResourcesBackend* backend, m_backends)
    {
        AbstractResource* res = backend->resourceByPackageName(name);
        if(res) {
            return res;
        }
    }
    return 0;
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
    app->backend()->installApplication(app);
}

void ResourcesModel::installApplication(AbstractResource* app, AddonList addons)
{
    app->backend()->installApplication(app, addons);
}

void ResourcesModel::removeApplication(AbstractResource* app)
{
    app->backend()->removeApplication(app);
}

void ResourcesModel::cancelTransaction(AbstractResource* app)
{
    app->backend()->cancelTransaction(app);
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
    MuonBackendsFactory f;
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
    MuonBackendsFactory f;
    addResourcesBackend(f.backend(name));
}

void ResourcesModel::integrateMainWindow(MuonMainWindow* w)
{
    Q_ASSERT(w->thread()==thread());
    m_mainwindow = w;
    setParent(w);
    foreach(AbstractResourcesBackend* b, m_backends) {
        b->integrateMainWindow(w);
    }
}

void ResourcesModel::resourceChangedByTransaction(Transaction* t)
{
    Q_ASSERT(!t->resource()->backend()->isFetching());
    QModelIndex idx = resourceIndex(t->resource());
    if(idx.isValid())
        dataChanged(idx, idx);
}

bool ResourcesModel::isFetching() const
{
    foreach(AbstractResourcesBackend* b, m_backends) {
        if(b->isFetching())
            return true;
    }
    return false;
}
