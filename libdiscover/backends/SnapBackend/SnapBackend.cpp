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
#include "SnapReviewsBackend.h"
#include <resources/StandardBackendUpdater.h>
#include <resources/SourcesModel.h>
#include <Category/Category.h>
#include <Transaction/Transaction.h>

#include <KAboutData>
#include <KLocalizedString>
#include <KPluginFactory>
#include <KConfigGroup>
#include <KSharedConfig>
#include <QDebug>
#include <QThread>
#include <QTimer>
#include <QAction>

#include "utils.h"

DISCOVER_BACKEND_PLUGIN(SnapBackend)

SnapBackend::SnapBackend(QObject* parent)
    : AbstractResourcesBackend(parent)
    , m_updater(new StandardBackendUpdater(this))
    , m_reviews(new SnapReviewsBackend(this))
{
    {
        auto request = m_client.connect();
        request->runSync();
        m_valid = request->error() == QSnapdRequest::NoError;
        if (!m_valid) {
            qWarning() << "snap problem at initialize:" << request->errorString();
            return;
        }
    }
    connect(m_reviews, &SnapReviewsBackend::ratingsReady, this, &AbstractResourcesBackend::emitRatingsReady);

    //make sure we populate the installed resources first
    populate(m_client.list(), AbstractResource::Installed);

    SourcesModel::global()->addBackend(this);
}

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
    } else if (filters.state >= AbstractResource::Installed) {
        return populate(m_client.list(), AbstractResource::Installed);
    } else {
        return populate(m_client.find(QSnapdClient::FindFlag::None, filters.search), AbstractResource::None);
    }
    return voidStream();
}

ResultsStream * SnapBackend::findResourceByPackageName(const QUrl& search)
{
    return search.scheme() == QLatin1String("snap") ? populate(m_client.listOne(search.host()), AbstractResource::None) : voidStream();
}

ResultsStream * SnapBackend::populate(QSnapdListOneRequest* job, AbstractResource::State state)
{
    auto stream = new ResultsStream(QStringLiteral("Snap-populateOne"));

    connect(job, &QSnapdFindRequest::complete, stream, [stream, this, state, job]() {
        if (job->error()) {
            qDebug() << "error:" << job->error() << job->errorString();
            return;
        }
        QSet<SnapResource*> higher = kFilter<QSet<SnapResource*>>(m_resources, [state](AbstractResource* res){ return res->state()>=state; });

        QSet<SnapResource*> resources;

        QSharedPointer<QSnapdSnap> snap(job->snap());
        const auto snapname = snap->name();
        SnapResource* res = m_resources.value(snapname);
        if (!res) {
            res = new SnapResource(snap, state, this);
            Q_ASSERT(res->packageName() == snapname);
            resources += res;
        } else {
            res->setState(state);
            higher.remove(res);
        }

        m_resources[res->packageName()] = res;
        for(auto res: higher) {
            res->setState(AbstractResource::None);
        }

        stream->resourcesFound({ret});
        stream->finish();
    });
    job->runAsync();
    return stream;
}

template <class T>
ResultsStream* SnapBackend::populate(T* job, AbstractResource::State state)
{
    auto stream = new ResultsStream(QStringLiteral("Snap-populate"));

    connect(job, &QSnapdFindRequest::complete, stream, [stream, this, state, job]() {
        QSet<SnapResource*> higher = kFilter<QSet<SnapResource*>>(m_resources, [state](AbstractResource* res){ return res->state()>=state; });

        QVector<AbstractResource*> ret;
        QSet<SnapResource*> resources;
        for (int i=0, c=job->snapCount(); i<c; ++i) {
            QSharedPointer<QSnapdSnap> snap(job->snap(i));
            const auto snapname = snap->name();
            SnapResource* res = m_resources.value(snapname);
            if (!res) {
                res = new SnapResource(snap, state, this);
                Q_ASSERT(res->packageName() == snapname);
                resources += res;
            } else {
                res->setState(state);
                higher.remove(res);
            }
            ret += res;
        }

        foreach(SnapResource* res, resources)
            m_resources[res->packageName()] = res;
        for(auto res: higher) {
            res->setState(AbstractResource::None);
        }

        if (!ret.isEmpty())
            stream->resourcesFound(ret);
        stream->finish();
    });
    job->runAsync();
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
    return m_reviews;
}

Transaction* SnapBackend::installApplication(AbstractResource* app, const AddonList& addons)
{
    Q_ASSERT(addons.isEmpty());
    return installApplication(app);
}

Transaction* SnapBackend::installApplication(AbstractResource* _app)
{
    auto app = qobject_cast<SnapResource*>(_app);
	return new SnapTransaction(app, m_client.install(app->packageName()), Transaction::InstallRole);
}

Transaction* SnapBackend::removeApplication(AbstractResource* _app)
{
    auto app = qobject_cast<SnapResource*>(_app);
	return new SnapTransaction(app, m_client.remove(app->packageName()), Transaction::RemoveRole);
}

QString SnapBackend::displayName() const
{
    return QStringLiteral("Snap");
}

void SnapBackend::refreshStates()
{
    populate(m_client.list(), AbstractResource::Installed);
}

#include "SnapBackend.moc"
