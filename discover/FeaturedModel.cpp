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

#include "discover_debug.h"
#include <QStandardPaths>
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QDir>
#include <KIO/StoredTransferJob>

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

    const bool isMobile = QByteArrayList{"1", "true"}.contains(qgetenv("QT_QUICK_CONTROLS_MOBILE"));
    auto fileName = isMobile ? QLatin1String("/featured-mobile-5.9.json") : QLatin1String("/featured-5.9.json");
    *featuredCache = dir + fileName;
    const QUrl featuredUrl(QStringLiteral("https://autoconfig.kde.org/discover") + fileName);
    auto *fetchJob = KIO::storedGet(featuredUrl, KIO::NoReload, KIO::HideProgressInfo);
    acquireFetching(true);
    connect(fetchJob, &KIO::StoredTransferJob::result, this, [this, fetchJob](){
        QFile f(*featuredCache);
        if (!f.open(QIODevice::WriteOnly))
            qCWarning(DISCOVER_LOG) << "could not open" << *featuredCache << f.errorString();
        f.write(fetchJob->data());
        f.close();
        refresh();
        acquireFetching(false);
    });

    if (!ResourcesModel::global()->backends().isEmpty() && QFile::exists(*featuredCache))
        refresh();
}

void FeaturedModel::refresh()
{
    acquireFetching(true);
    QFile f(*featuredCache);
    if (!f.open(QIODevice::ReadOnly)) {
        qCWarning(DISCOVER_LOG) << "couldn't open file" << *featuredCache << f.errorString();
        return;
    }
    QJsonParseError error;
    const auto array = QJsonDocument::fromJson(f.readAll(), &error).array();
    if (error.error) {
        qCWarning(DISCOVER_LOG) << "couldn't parse" << *featuredCache << ". error:" << error.errorString();
        return;
    }

    const auto uris = kTransform<QVector<QUrl>>(array, [](const QJsonValue& uri) { return QUrl(uri.toString()); });
    setUris(uris);
}

void FeaturedModel::setUris(const QVector<QUrl>& uris)
{
    acquireFetching(false);
    auto backend = ResourcesModel::global()->currentApplicationBackend();
    if (uris == m_uris || !backend)
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
    const auto appsBackend = ResourcesModel::global()->currentApplicationBackend();
    QHash<QString, AbstractResource*> resById;
    for(auto it = resources.begin(); it!=resources.end(); ) {
        auto id = (*it)->appstreamId();
        auto curr = resById.value(id);
        if (curr && curr->backend() == appsBackend) {
            it = resources.erase(it);
        } else {
            resources.removeAll(curr);
            resById[id] = *it;
            ++it;
        }
    }
}

void FeaturedModel::acquireFetching(bool f)
{
    if (f)
        m_isFetching++;
    else
        m_isFetching--;

    if ((!f && m_isFetching==0) || (f && m_isFetching==1)) {
        emit isFetchingChanged();
    }
    Q_ASSERT(m_isFetching>=0);
}

void FeaturedModel::setResources(const QVector<AbstractResource *>& _resources)
{
    auto resources = _resources;
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

    beginRemoveRows({}, index, index);
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
