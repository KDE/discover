/*
 *   SPDX-FileCopyrightText: 2016 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *   SPDX-FileCopyrightText: 2021 Carl Schwan <carlschwan@kde.org>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "FeaturedModel.h"

#include "discover_debug.h"
#include <FeaturedModel.moc>
#include <KIO/StoredTransferJob>
#include <KLocalizedString>
#include <QAbstractListModel>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QStandardPaths>
#include <QtGlobal>

#include <resources/ResourcesModel.h>
#include <resources/StoredResultsStream.h>
#include <utils.h>

Q_GLOBAL_STATIC(QString, featuredCache)


FeaturedModel::FeaturedModel(QObject *parent)
    : QAbstractListModel(parent)
    , m_specialAppsModel(new SpecialAppsModel(this))
{
    connect(ResourcesModel::global(), &ResourcesModel::currentApplicationBackendChanged, this, &FeaturedModel::refreshCurrentApplicationBackend);

    const QString dir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
    QDir().mkpath(dir);

    const bool isMobile = QByteArrayList{"1", "true"}.contains(qgetenv("QT_QUICK_CONTROLS_MOBILE"));
    auto fileName = isMobile ? QLatin1String("/featured-mobile-5.23.json") : QLatin1String("/featured-5.23.json");
    *featuredCache = dir + fileName;
//     const QUrl featuredUrl(QStringLiteral("https://autoconfig.kde.org/discover") + fileName);
    const QUrl featuredUrl(QStringLiteral("file:///home/fhek789/Downloads/featured.json"));
    auto *fetchJob = KIO::storedGet(featuredUrl, KIO::NoReload, KIO::HideProgressInfo);
    acquireFetching(true);
    connect(fetchJob, &KIO::StoredTransferJob::result, this, [this, fetchJob]() {
        const auto dest = qScopeGuard([this] {
            acquireFetching(false);
            refresh();
        });
        if (fetchJob->error() != 0)
            return;

        QFile f(*featuredCache);
        if (!f.open(QIODevice::WriteOnly))
            qCWarning(DISCOVER_LOG) << "could not open" << *featuredCache << f.errorString();
        f.write(fetchJob->data());
        f.close();
    });

    refreshCurrentApplicationBackend();
}

void FeaturedModel::refreshCurrentApplicationBackend()
{
    auto backend = ResourcesModel::global()->currentApplicationBackend();
    if (m_backend == backend)
        return;

    if (m_backend) {
        disconnect(m_backend, &AbstractResourcesBackend::fetchingChanged, this, &FeaturedModel::refresh);
        disconnect(m_backend, &AbstractResourcesBackend::resourceRemoved, this, &FeaturedModel::removeResource);
    }

    m_backend = backend;

    if (backend) {
        connect(backend, &AbstractResourcesBackend::fetchingChanged, this, &FeaturedModel::refresh);
        connect(backend, &AbstractResourcesBackend::resourceRemoved, this, &FeaturedModel::removeResource);
    }

    if (backend && QFile::exists(*featuredCache))
        refresh();

    Q_EMIT currentApplicationBackendChanged(m_backend);
}

void FeaturedModel::refresh()
{
    // usually only useful if launching just fwupd or kns backends
    if (!m_backend)
        return;

    acquireFetching(true);
    const auto dest = qScopeGuard([this] {
        acquireFetching(false);
    });
    QFile f(*featuredCache);
    if (!f.open(QIODevice::ReadOnly)) {
        qCWarning(DISCOVER_LOG) << "couldn't open file" << *featuredCache << f.errorString();
        return;
    }
    QJsonParseError error;
    const auto object = QJsonDocument::fromJson(f.readAll(), &error).object();
    if (error.error) {
        qCWarning(DISCOVER_LOG) << "couldn't parse" << *featuredCache << ". error:" << error.errorString();
        return;
    }

    QHash<QString, QVector<QUrl>> urisCategory;
    const auto categories = object[QStringLiteral("categories")].toObject();
    for (const auto &name : categories.keys()) {
        const auto category = categories[name];
        urisCategory[name] = kTransform<QVector<QUrl>>(categories[name].toArray(), [](const QJsonValue &uri) {
            return QUrl(uri.toString());
        });
    }

    QVector<FeaturedApp> apps;
    for (const auto &app : object[QStringLiteral("featured")].toArray()) {
        const auto appObject = app.toObject();
        apps.append(FeaturedApp{QUrl(appObject[QStringLiteral("id")].toString())});
    }

    setUris(urisCategory, apps);
}

template<typename T>
class asKeyValueRange
{
public:
    asKeyValueRange(T &data)
        : m_data{data}
    {
    }
    auto begin()
    {
        return m_data.keyValueBegin();
    }
    auto end()
    {
        return m_data.keyValueEnd();
    }

private:
    T &m_data;
};

static void filterDupes(QVector<AbstractResource *> &resources)
{
    QSet<QString> found;
    for (auto it = resources.begin(); it != resources.end();) {
        auto id = (*it)->appstreamId();
        if (found.contains(id)) {
            it = resources.erase(it);
        } else {
            found.insert(id);
            ++it;
        }
    }
}

void FeaturedModel::setUris(const QHash<QString, QVector<QUrl>> &uris, const QVector<FeaturedApp> &featuredApps)
{
    if (!m_backend) {
        return;
    }

    QSet<ResultsStream *> streams;
    for (const auto &featuredApp : featuredApps) {
        AbstractResourcesBackend::Filters filter;
        filter.resourceUrl = featuredApp.id;
        streams << m_backend->search(filter);
    }
    if (!streams.isEmpty()) {
        auto stream = new StoredResultsStream(streams);
        acquireFetching(true);
        connect(stream, &StoredResultsStream::finishedResources, this, [this, featuredApps](const QVector<AbstractResource *> &_resources) {
            QVector<AbstractResource *> abstractResources = _resources;

            filterDupes(abstractResources);
            QVector<AbstractResource *> resources;

            // O(n^2) complexity that could theorically be improved but it's a small n
            for (const auto &resource : abstractResources) {
                for (const auto &app : featuredApps) {
                    if (app.id == resource->url()) {
                        resources.append(resource);
                        break;
                    }
                }
            }

            m_specialAppsModel->setResources(resources);
        });
    }

    for (const auto [name, category] : asKeyValueRange(uris)) {
        QSet<ResultsStream *> streams;
        for (const auto &uri : category) {
            AbstractResourcesBackend::Filters filter;
            filter.resourceUrl = uri;
            streams << m_backend->search(filter);
        }
        if (!streams.isEmpty()) {
            auto stream = new StoredResultsStream(streams);
            acquireFetching(true);
            connect(stream, &StoredResultsStream::finishedResources, this, [name, this](const QVector<AbstractResource *> &resources) {
                setResources(name, resources);
            });
        }
    }
}

void FeaturedModel::acquireFetching(bool f)
{
    if (f)
        m_isFetching++;
    else
        m_isFetching--;

    if ((!f && m_isFetching == 0) || (f && m_isFetching == 1)) {
        Q_EMIT isFetchingChanged();
    }
    Q_ASSERT(m_isFetching >= 0);
}

void FeaturedModel::setResources(const QString &category, const QVector<AbstractResource *> &_resources)
{
    auto resources = _resources;
    filterDupes(resources);

    int i = 0;
    while (i < m_resources.count() && m_resources[i].first != category) {
        i++;
    }

    if (i == m_resources.count()) {
        // new category
        beginInsertRows({}, i, i);
        m_resources.append({category, resources});
        endInsertRows();

        beginInsertRows(index(i, 0), 0, m_resources[i].second.count() - 1);
        endInsertRows();
    } else if (m_resources[i].second != resources) {
        // existing category
        beginRemoveRows(index(i, 0), 0, m_resources[i].second.count());
        m_resources[i] = {category, resources};
        endRemoveRows();
        beginInsertRows(index(i, 0), 0, m_resources[i].second.count() - 1);
        endInsertRows();
    }

    acquireFetching(false);
}

void FeaturedModel::removeResource(AbstractResource *resource)
{
    for (int category = 0; category < m_resources.count(); category++) {
        int i = m_resources[category].second.indexOf(resource);
        if (i < 0) {
            continue;
        }
        beginRemoveRows(index(category, 0), i, i);
        m_resources[category].second.removeAt(i);
        endRemoveRows();
    }
}

QVariant FeaturedModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) {
        return {};
    }

    if (index.parent().isValid()) {
        // application
        if (role != Qt::UserRole) {
            return {};
        }
        auto res = m_resources[index.parent().row()].second.value(index.row());
        if (!res) {
            return {};
        }

        return QVariant::fromValue<QObject*>(res);
    }

    // category
    if (role == Qt::DisplayRole) {
        const auto categoryName = m_resources[index.row()].first;
        if (categoryName == QStringLiteral("graphics")) {
            return i18n("Graphics");
        } else if (categoryName == QStringLiteral("office")) {
            return i18n("Office");
        } else if (categoryName == QStringLiteral("games")) {
            return i18n("Games");
        } else if (categoryName == QStringLiteral("development")) {
            return i18n("Development");
        } else if (categoryName == QStringLiteral("education")) {
            return i18n("Education");
        }
    }
    return {};
}

QModelIndex FeaturedModel::index(int row, int column, const QModelIndex &parent) const
{
    if (parent.isValid()) {
        return createIndex(row, column, (intptr_t)parent.row() + 1);
    }
    return createIndex(row, column, nullptr);
}

QModelIndex FeaturedModel::parent(const QModelIndex &child) const
{
    if (child.internalId()) {
        return createIndex(child.internalId() - 1, 0, nullptr);
    }
    return QModelIndex();
}

int FeaturedModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        return m_resources[parent.row()].second.count();
    }
    return m_resources.count();
}

int FeaturedModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return 0;
}

bool FeaturedModel::hasChildren(const QModelIndex& index) const
{
    if (index.parent().isValid()) {
        return false;
    }
    return m_resources.count() > 0;
}

QHash<int, QByteArray> FeaturedModel::roleNames() const
{
    return {
        {Qt::DisplayRole, QByteArrayLiteral("categoryName")},
        {Qt::UserRole, QByteArrayLiteral("applicationObject")}
    };
}

QAbstractItemModel *FeaturedModel::specialApps() const
{
    return m_specialAppsModel;
}
