/***************************************************************************
 *   Copyright © 2017 Aleix Pol Gonzalez <aleixpol@blue-systems.com>       *
 *   Copyright © 2017 Jan Grulich <jgrulich@redhat.com>                    *
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
    QStorageInfo storageInfo(cacheDir);
    cache->setCacheDirectory(cacheDir);
    cache->setMaximumCacheSize(storageInfo.bytesTotal() / 1000);
    setCache(cache);
}

QNetworkReply * CachedNetworkAccessManager::createRequest(Operation op, const QNetworkRequest &request, QIODevice *outgoingData)
{
    QNetworkRequest req(request);
    req.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::PreferCache);
    return QNetworkAccessManager::createRequest(op, request, outgoingData);
}

QNetworkAccessManager * CachedNetworkAccessManagerFactory::create(QObject *parent)
{
    return new CachedNetworkAccessManager(QStringLiteral("images"), parent);
}

