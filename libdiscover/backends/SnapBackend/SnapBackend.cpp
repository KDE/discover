/***************************************************************************
 *   Copyright © 2013 Aleix Pol Gonzalez <aleixpol@blue-systems.com>       *
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

#include "SnapBackend.h"
#include "SnapTransaction.h"
#include "SnapResource.h"
#include "appstream/AppStreamIntegration.h"
#include <appstream/AppStreamUtils.h>
#include <resources/StandardBackendUpdater.h>
#include <resources/SourcesModel.h>
#include <Category/Category.h>
#include <Transaction/Transaction.h>
#include <resources/StoredResultsStream.h>

#include <KAboutData>
#include <KLocalizedString>
#include <KPluginFactory>
#include <KConfigGroup>
#include <KSharedConfig>
#include <QDebug>
#include <QThread>
#include <QTimer>
#include <QAction>
#include <QStandardItemModel>

#include "utils.h"

DISCOVER_BACKEND_PLUGIN(SnapBackend)

class SnapSourcesBackend : public AbstractSourcesBackend
{
public:
    explicit SnapSourcesBackend(AbstractResourcesBackend * parent) : AbstractSourcesBackend(parent), m_model(new QStandardItemModel(this)) {
        auto it = new QStandardItem(i18n("Snap"));
        it->setData(QStringLiteral("Snap"), IdRole);
        m_model->appendRow(it);
    }

    QAbstractItemModel* sources() override { return m_model; }
    bool addSource(const QString& /*id*/) override { return false; }
    bool removeSource(const QString& /*id*/) override { return false;}
    QString idDescription() override { return QStringLiteral("Snap"); }
    QVariantList actions() const override { return {}; }

    bool supportsAdding() const override { return false; }
    bool canMoveSources() const override { return false; }

private:
    QStandardItemModel* const m_model;
};

SnapBackend::SnapBackend(QObject* parent)
    : AbstractResourcesBackend(parent)
    , m_updater(new StandardBackendUpdater(this))
    , m_reviews(AppStreamIntegration::global()->reviews())
{
    connect(m_reviews.data(), &OdrsReviewsBackend::error, this, &SnapBackend::passiveMessage);
    connect(m_reviews.data(), &OdrsReviewsBackend::ratingsReady, this, [this] {
        m_reviews->emitRatingFetched(this, kTransform<QList<AbstractResource*>>(m_resources.values(), [] (AbstractResource* r) { return r; }));
    });

    //make sure we populate the installed resources first
    refreshStates();

    SourcesModel::global()->addSourcesBackend(new SnapSourcesBackend(this));
}

SnapBackend::~SnapBackend() = default;

int SnapBackend::updatesCount() const
{
    return m_updater->updatesCount();
}

static ResultsStream* voidStream() { return new ResultsStream(QStringLiteral("Snap-void"), {}); }

ResultsStream * SnapBackend::search(const AbstractResourcesBackend::Filters& filters)
{
    if (!filters.extends.isEmpty()) {
        return voidStream();
    } else if (!filters.resourceUrl.isEmpty()) {
        return findResourceByPackageName(filters.resourceUrl);
    } else if (filters.category && filters.category->isAddons()) {
        return voidStream();
    } else if (filters.state >= AbstractResource::Installed || filters.origin == QLatin1String("Snap")) {
        std::function<bool(const QSharedPointer<QSnapdSnap>&)> f = [filters](const QSharedPointer<QSnapdSnap>& s) { return filters.search.isEmpty() || s->name().contains(filters.search, Qt::CaseInsensitive) || s->description().contains(filters.search, Qt::CaseInsensitive); };
        return populateWithFilter(m_client.getSnaps(), f);
    } else if (!filters.search.isEmpty()) {
        return populate(m_client.find(QSnapdClient::FindFlag::None, filters.search));
    }
    return voidStream();
}

ResultsStream * SnapBackend::findResourceByPackageName(const QUrl& search)
{
    Q_ASSERT(!search.host().isEmpty() || !AppStreamUtils::appstreamIds(search).isEmpty());
    return search.scheme() == QLatin1String("snap")      ? populate(m_client.find(QSnapdClient::MatchName, search.host())) :
#ifdef SNAP_FIND_COMMON_ID
           search.scheme() == QLatin1String("appstream") ? populate(kTransform<QVector<QSnapdFindRequest*>>(AppStreamUtils::appstreamIds(search),
                                                                                [this] (const QString &id) {return m_client.find(QSnapdClient::MatchCommonId, id); })) :
#endif
                voidStream();
}

template <class T>
ResultsStream* SnapBackend::populate(T* job)
{
    return populate<T>(QVector<T*>{job});
}

template <class T>
ResultsStream* SnapBackend::populate(const QVector<T*>& jobs)
{
    std::function<bool(const QSharedPointer<QSnapdSnap>&)> acceptAll = [](const QSharedPointer<QSnapdSnap>&){ return true; };
    return populateJobsWithFilter(jobs, acceptAll);
}

template <class T>
ResultsStream* SnapBackend::populateWithFilter(T* job, std::function<bool(const QSharedPointer<QSnapdSnap>& s)>& filter)
{
    return populateJobsWithFilter<T>({job}, filter);
}

template <class T>
ResultsStream* SnapBackend::populateJobsWithFilter(const QVector<T*>& jobs, std::function<bool(const QSharedPointer<QSnapdSnap>& s)>& filter)
{
    auto stream = new ResultsStream(QStringLiteral("Snap-populate"));
    stream->setProperty("remaining", jobs.count());
    for(auto job : jobs) {
        connect(job, &T::complete, stream, [stream, this, job, filter]() {
            const int remaining = stream->property("remaining").toInt() - 1;
            stream->setProperty("remaining", remaining);

            if (job->error()) {
                qDebug() << "error:" << job->error() << job->errorString();
                if (remaining == 0)
                    stream->finish();
                return;
            }

            QVector<AbstractResource*> ret;
            QVector<SnapResource*> resources;
            ret.reserve(job->snapCount());
            resources.reserve(job->snapCount());
            for (int i=0, c=job->snapCount(); i<c; ++i) {
                QSharedPointer<QSnapdSnap> snap(job->snap(i));

                if (!filter(snap))
                    continue;

                const auto snapname = snap->name();
                SnapResource* res = m_resources.value(snapname);
                if (!res) {
                    res = new SnapResource(snap, AbstractResource::None, this);
                    Q_ASSERT(res->packageName() == snapname);
                    resources += res;
                } else {
                    res->setSnap(snap);
                }
                ret += res;
            }

            foreach(SnapResource* res, resources)
                m_resources[res->packageName()] = res;

            if (!ret.isEmpty())
                Q_EMIT stream->resourcesFound(ret);

            if (remaining == 0)
                stream->finish();
        });
        job->runAsync();
    }
    return stream;
}

void SnapBackend::setFetching(bool fetching)
{
    if (m_fetching != fetching) {
        m_fetching = fetching;
        Q_EMIT fetchingChanged();
    } else {
        qWarning() << "fetching already on state" << fetching;
    }
}

AbstractBackendUpdater* SnapBackend::backendUpdater() const
{
    return m_updater;
}

AbstractReviewsBackend* SnapBackend::reviewsBackend() const
{
    return m_reviews.data();
}

Transaction* SnapBackend::installApplication(AbstractResource* app, const AddonList& addons)
{
    Q_ASSERT(addons.isEmpty());
    return installApplication(app);
}

Transaction* SnapBackend::installApplication(AbstractResource* _app)
{
    auto app = qobject_cast<SnapResource*>(_app);
	return new SnapTransaction(&m_client, app, Transaction::InstallRole, AbstractResource::Installed);
}

Transaction* SnapBackend::removeApplication(AbstractResource* _app)
{
    auto app = qobject_cast<SnapResource*>(_app);
	return new SnapTransaction(&m_client, app, Transaction::RemoveRole, AbstractResource::None);
}

QString SnapBackend::displayName() const
{
    return QStringLiteral("Snap");
}

void SnapBackend::refreshStates()
{
    auto ret = new StoredResultsStream({populate(m_client.getSnaps())});
    connect(ret, &StoredResultsStream::finishedResources, this, [this] (const QVector<AbstractResource*>& resources){
        for (auto res: qAsConst(m_resources)) {
            if (resources.contains(res))
                res->setState(AbstractResource::Installed);
            else
                res->setState(AbstractResource::None);
        }
    });
}

#include "SnapBackend.moc"
