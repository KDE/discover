/***************************************************************************
 *   Copyright © 2016 Aleix Pol Gonzalez <aleixpol@blue-systems.com>       *
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

#include "FeaturedModel.h"

#include <QDebug>
#include <QStandardPaths>
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QDir>
#include <KIO/FileCopyJob>

#include <utils.h>
#include <resources/ResourcesModel.h>
#include <resources/StoredResultsStream.h>

Q_GLOBAL_STATIC(QString, featuredCache)

FeaturedModel::FeaturedModel()
{
    connect(ResourcesModel::global(), &ResourcesModel::backendsChanged, this, &FeaturedModel::refresh);
    connect(ResourcesModel::global(), &ResourcesModel::currentApplicationBackendChanged, this, &FeaturedModel::refresh);
    connect(ResourcesModel::global(), &ResourcesModel::fetchingChanged, this, &FeaturedModel::refresh);
    connect(ResourcesModel::global(), &ResourcesModel::resourceRemoved, this, &FeaturedModel::removeResource);

    const QString dir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
    QDir().mkpath(dir);
    *featuredCache = dir+QLatin1String("/featured-5.9.json");

    const QUrl featuredUrl(QStringLiteral("https://autoconfig.kde.org/discover/featured-5.9.json"));
    KIO::FileCopyJob *getJob = KIO::file_copy(featuredUrl, QUrl::fromLocalFile(*featuredCache), -1, KIO::Overwrite | KIO::HideProgressInfo);
    connect(getJob, &KIO::FileCopyJob::result, this, &FeaturedModel::refresh);

    if (!ResourcesModel::global()->backends().isEmpty() && QFile::exists(*featuredCache))
        refresh();
}

void FeaturedModel::refresh()
{
    QFile f(*featuredCache);
    if (!f.open(QIODevice::ReadOnly)) {
        qWarning() << "couldn't open file" << *featuredCache;
        return;
    }
    QJsonParseError error;
    const auto array = QJsonDocument::fromJson(f.readAll(), &error).array();
    if (error.error) {
        qWarning() << "couldn't parse" << *featuredCache << ". error:" << error.errorString();
        return;
    }

    const auto uris = kTransform<QVector<QUrl>>(array, [](const QJsonValue& uri) { return QUrl(uri.toString()); });
    setUris(uris);
}

void FeaturedModel::setUris(const QVector<QUrl>& uris)
{
    auto backend = ResourcesModel::global()->currentApplicationBackend();
    if (uris == m_uris || !backend || backend->isFetching())
        return;

    QSet<ResultsStream*> streams;
    foreach(const auto &uri, uris) {
        AbstractResourcesBackend::Filters filter;
        filter.resourceUrl = uri;
        streams << backend->search(filter);
    }
    auto stream = new StoredResultsStream(streams);
    connect(stream, &StoredResultsStream::finishedResources, this, &FeaturedModel::setResources);
}

static void filterDupes(QVector<AbstractResource *> &resources)
{
    QSet<QString> uri, dupeUri;
    for(auto res: qAsConst(resources)) {
        const auto id = res->appstreamId();
        if (uri.contains(id))
            dupeUri += id;
        else
            uri += id;
    }

    const auto appBackend = ResourcesModel::global()->currentApplicationBackend();
    for(auto it = resources.begin(); it != resources.end(); ) {
        const auto backend = (*it)->backend();
        if (backend != appBackend && dupeUri.contains((*it)->appstreamId()))
            it = resources.erase(it);
        else
            ++it;
    }
}

void FeaturedModel::setResources(const QVector<AbstractResource *>& _resources)
{
    auto resources = _resources;
    qDebug() << "mup" << resources;
    filterDupes(resources);

    if (m_resources == resources)
        return;

    //TODO: sort like in the json files

    beginResetModel();
    m_resources = resources;
    endResetModel();
}

void FeaturedModel::removeResource(AbstractResource* resource)
{
    int index = m_resources.indexOf(resource);
    if (index<0)
        return;

    beginRemoveRows({}, index, 0);
    m_resources.removeAt(index);
    endRemoveRows();
}

QVariant FeaturedModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || role!=Qt::UserRole)
        return {};

    auto res = m_resources.value(index.row());
    if (!res)
        return {};

    return QVariant::fromValue<QObject*>(res);
}

int FeaturedModel::rowCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : m_resources.count();
}

QHash<int, QByteArray> FeaturedModel::roleNames() const
{
    return {{Qt::UserRole, "application"}};
}
