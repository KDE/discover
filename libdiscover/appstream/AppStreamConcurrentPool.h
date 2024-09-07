/*
 *   SPDX-FileCopyrightText: 2024 Aleix Pol Gonzalez <aleixpol@kde.org>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include <QFuture>
#include <QMutex>
#include <QPointer>

#include <AppStreamQt/pool.h>

#include "discovercommon_export.h"

namespace AppStream
{

/**
 * Convenience façade class to have QtConcurrent-enabled pools
 */
class DISCOVERCOMMON_EXPORT ConcurrentPool : public QObject
{
    Q_OBJECT
public:
    /**
     * Tells which @p pool to use and in which thread pool the jobs will be run
     *
     * @param pool takes ownership
     * @param threadPool does not take ownership
     */
    void reset(AppStream::Pool *pool, QThreadPool *threadPool);

    QFuture<ComponentBox> search(const QString &term);

    QFuture<ComponentBox> components();

    QFuture<ComponentBox> componentsById(const QString &cid);

    QFuture<ComponentBox> componentsByProvided(Provided::Kind kind, const QString &item);

    QFuture<ComponentBox> componentsByKind(Component::Kind kind);

    QFuture<ComponentBox> componentsByCategories(const QStringList &categories);

    QFuture<ComponentBox> componentsByLaunchable(Launchable::Kind kind, const QString &value);

    QFuture<ComponentBox> componentsByExtends(const QString &extendedId);

    QFuture<ComponentBox> componentsByBundleId(Bundle::Kind kind, const QString &bundleId, bool matchPrefix);

    QString lastError();
    void loadAsync();

    AppStream::Pool *get() const
    {
        return m_pool.get();
    }

Q_SIGNALS:
    void loadFinished(bool success);

private:
    QMutex m_mutex;
    std::unique_ptr<AppStream::Pool> m_pool;
    QPointer<QThreadPool> m_threadPool;
};

}
