/*
 *   SPDX-FileCopyrightText: 2024 Aleix Pol Gonzalez <aleixpol@kde.org>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "AppStreamConcurrentPool.h"

#include <QtConcurrentRun>

using namespace AppStream;

void ConcurrentPool::reset(AppStream::Pool *pool, QThreadPool *threadPool)
{
    m_pool.reset(pool);
    connect(pool, &Pool::loadFinished, this, &ConcurrentPool::loadFinished);

    m_threadPool = threadPool;
}

void ConcurrentPool::loadAsync()
{
    QMutexLocker lock(&m_mutex);
    return m_pool->loadAsync();
}

QString ConcurrentPool::lastError()
{
    QMutexLocker lock(&m_mutex);
    return m_pool->lastError();
}

QFuture<ComponentBox> ConcurrentPool::search(const QString &term)
{
    return QtConcurrent::run(m_threadPool.get(), [this, term] {
        QMutexLocker lock(&m_mutex);
        return m_pool->search(term);
    });
}

QFuture<ComponentBox> ConcurrentPool::components()
{
    return QtConcurrent::run(m_threadPool.get(), [this] {
        QMutexLocker lock(&m_mutex);
        return m_pool->components();
    });
}

QFuture<ComponentBox> ConcurrentPool::componentsById(const QString &cid)
{
    return QtConcurrent::run(m_threadPool.get(), [this, cid] {
        QMutexLocker lock(&m_mutex);
        return m_pool->componentsById(cid);
    });
}

QFuture<ComponentBox> ConcurrentPool::componentsByProvided(Provided::Kind kind, const QString &item)
{
    return QtConcurrent::run(m_threadPool.get(), [this, kind, item] {
        QMutexLocker lock(&m_mutex);
        return m_pool->componentsByProvided(kind, item);
    });
}

QFuture<ComponentBox> ConcurrentPool::componentsByKind(Component::Kind kind)
{
    return QtConcurrent::run(m_threadPool.get(), [this, kind] {
        QMutexLocker lock(&m_mutex);
        return m_pool->componentsByKind(kind);
    });
}

QFuture<ComponentBox> ConcurrentPool::componentsByCategories(const QStringList &categories)
{
    return QtConcurrent::run(m_threadPool.get(), [this, categories] {
        QMutexLocker lock(&m_mutex);
        return m_pool->componentsByCategories(categories);
    });
}

QFuture<ComponentBox> ConcurrentPool::componentsByLaunchable(Launchable::Kind kind, const QString &value)
{
    return QtConcurrent::run(m_threadPool.get(), [this, kind, value] {
        QMutexLocker lock(&m_mutex);
        return m_pool->componentsByLaunchable(kind, value);
    });
}

QFuture<ComponentBox> ConcurrentPool::componentsByExtends(const QString &extendedId)
{
    return QtConcurrent::run(m_threadPool.get(), [this, extendedId] {
        QMutexLocker lock(&m_mutex);
        return m_pool->componentsByExtends(extendedId);
    });
}
QFuture<ComponentBox> ConcurrentPool::componentsByBundleId(Bundle::Kind kind, const QString &bundleId, bool matchPrefix)
{
    return QtConcurrent::run(m_threadPool.get(), [this, kind, bundleId, matchPrefix] {
        QMutexLocker lock(&m_mutex);
        return m_pool->componentsByBundleId(kind, bundleId, matchPrefix);
    });
}
