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
    : QObject(parent)
    , m_initializingBackends(0)
    , m_actionCollection(nullptr)
{
    init(load);
    connect(this, &ResourcesModel::allInitialized, this, &ResourcesModel::fetchingChanged);
}

void ResourcesModel::init(bool load)
{
    Q_ASSERT(!s_self);
    Q_ASSERT(QCoreApplication::instance()->thread()==QThread::currentThread());

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

void ResourcesModel::addResourcesBackend(AbstractResourcesBackend* backend)
{
    Q_ASSERT(!m_backends.contains(backend));
    if(!backend->isValid()) {
        qWarning() << "Discarding invalid backend" << backend->name();
        CategoryModel::blacklistPlugin(backend->name());
        backend->deleteLater();
        return;
    }

    m_backends += backend;
    if(!backend->isFetching()) {
        if (backend->updatesCount() > 0)
            emit updatesCountChanged();
    } else {
        m_initializingBackends++;
    }
    if(m_actionCollection)
        backend->integrateActions(m_actionCollection);

    connect(backend, &AbstractResourcesBackend::fetchingChanged, this, &ResourcesModel::callerFetchingChanged);
    connect(backend, &AbstractResourcesBackend::allDataChanged, this, &ResourcesModel::updateCaller);
    connect(backend, &AbstractResourcesBackend::resourcesChanged, this, &ResourcesModel::resourceDataChanged);
    connect(backend, &AbstractResourcesBackend::updatesCountChanged, this, &ResourcesModel::updatesCountChanged);
    connect(backend, &AbstractResourcesBackend::resourceRemoved, this, &ResourcesModel::resourceRemoved);
    connect(backend, &AbstractResourcesBackend::passiveMessage, this, &ResourcesModel::passiveMessage);

    if(m_initializingBackends==0)
        emit allInitialized();
    else
        emit fetchingChanged();
}

void ResourcesModel::callerFetchingChanged()
{
    AbstractResourcesBackend* backend = qobject_cast<AbstractResourcesBackend*>(sender());

    if (!backend->isValid()) {
        qWarning() << "Discarding invalid backend" << backend->name();
        int idx = m_backends.indexOf(backend);
        Q_ASSERT(idx>=0);
        m_backends.removeAt(idx);
        CategoryModel::blacklistPlugin(backend->name());
        backend->deleteLater();
        return;
    }

    if(backend->isFetching()) {
        m_initializingBackends++;
        emit fetchingChanged();
    } else {
        m_initializingBackends--;
        if(m_initializingBackends==0)
            emit allInitialized();
        else
            emit fetchingChanged();
    }
}

void ResourcesModel::updateCaller(const QVector<QByteArray>& properties)
{
    AbstractResourcesBackend* backend = qobject_cast<AbstractResourcesBackend*>(sender());
    
    Q_EMIT backendDataChanged(backend, properties);
}

QVector< AbstractResourcesBackend* > ResourcesModel::backends() const
{
    return m_backends;
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

void ResourcesModel::registerAllBackends()
{
    DiscoverBackendsFactory f;
    const auto backends = f.allBackends();
    if(m_initializingBackends==0 && backends.isEmpty()) {
        qWarning() << "Couldn't find any backends";
        emit allInitialized();
    } else {
        foreach(AbstractResourcesBackend* b, backends) {
            addResourcesBackend(b);
        }
        emit backendsChanged();
    }
}

void ResourcesModel::registerBackendByName(const QString& name)
{
    DiscoverBackendsFactory f;
    for(auto b : f.backend(name))
        addResourcesBackend(b);

    emit backendsChanged();
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
    bool ret = true;
    foreach (AbstractResourcesBackend* backend, m_backends) {
        ret = backend->extends().contains(id);
        if (ret)
            break;
    }
    return ret;
}

AggregatedResultsStream::AggregatedResultsStream(const QSet<ResultsStream*>& streams)
    : ResultsStream(QStringLiteral("AggregatedResultsStream"))
{
    if (streams.isEmpty()) {
        qWarning() << "no streams to aggregate!!";
        destruction(nullptr);
    }

    for (auto stream: streams) {
        connect(stream, &ResultsStream::resourcesFound, this, &AggregatedResultsStream::addResults);
        connect(stream, &QObject::destroyed, this, &AggregatedResultsStream::destruction);
        m_streams << stream;
    }

    m_delayedEmission.setInterval(0);
    connect(&m_delayedEmission, &QTimer::timeout, this, &AggregatedResultsStream::emitResults);
}

void AggregatedResultsStream::addResults(const QVector<AbstractResource *>& res)
{
     m_results += res;

     m_delayedEmission.start();
}

void AggregatedResultsStream::emitResults()
{
    if (!m_results.isEmpty()) {
        Q_EMIT resourcesFound(m_results);
        m_results.clear();
    }
    m_delayedEmission.setInterval(m_delayedEmission.interval() + 100);
    m_delayedEmission.stop();
}

void AggregatedResultsStream::destruction(QObject* obj)
{
    m_streams.remove(obj);
    if (m_streams.isEmpty()) {
        emitResults();
        Q_EMIT finished();
        deleteLater();
    }
}

AggregatedResultsStream * ResourcesModel::findResourceByPackageName(const QString& search)
{
    QSet<ResultsStream*> streams;
    foreach(auto backend, m_backends) {
        streams << backend->findResourceByPackageName(search);
    }
    return new AggregatedResultsStream(streams);
}

AggregatedResultsStream* ResourcesModel::search(const AbstractResourcesBackend::Filters& search)
{
    QSet<ResultsStream*> streams;
    foreach(auto backend, m_backends) {
        streams << backend->search(search);
    }
    return new AggregatedResultsStream(streams);
}

AbstractResource* ResourcesModel::resourceForFile(const QUrl& file)
{
    AbstractResource* ret = nullptr;
    foreach(auto backend, m_backends) {
        ret = backend->resourceForFile(file);
        if (ret)
            break;
    }
    return ret;
}
