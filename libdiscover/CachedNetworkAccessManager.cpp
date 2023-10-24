/*
 *   SPDX-FileCopyrightText: 2017 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *   SPDX-FileCopyrightText: 2017 Jan Grulich <jgrulich@redhat.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "CachedNetworkAccessManager.h"

#include <QNetworkDiskCache>
#include <QNetworkRequest>
#include <QStandardPaths>
#include <QStorageInfo>

CachedNetworkAccessManager::CachedNetworkAccessManager(const QString &path, QObject *parent)
    : QNetworkAccessManager(parent)
{
    const QString cacheDir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + QLatin1Char('/') + path;
    QNetworkDiskCache *cache = new QNetworkDiskCache(this);
    QStorageInfo storageInfo(QStandardPaths::writableLocation(QStandardPaths::CacheLocation));
    cache->setCacheDirectory(cacheDir);
    cache->setMaximumCacheSize(storageInfo.bytesTotal() / 1000);
    setCache(cache);

    setTransferTimeout();
}

QNetworkReply *CachedNetworkAccessManager::createRequest(Operation op, const QNetworkRequest &request, QIODevice *outgoingData)
{
    QNetworkRequest req(request);
    req.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::PreferCache);
    return QNetworkAccessManager::createRequest(op, request, outgoingData);
}
