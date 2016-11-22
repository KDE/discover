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
#include <Transaction/Transaction.h>
#include <Transaction/TransactionModel.h>

#include <KAboutData>
#include <KLocalizedString>
#include <KPluginFactory>
#include <KConfigGroup>
#include <KSharedConfig>
#include <QDebug>
#include <QThread>
#include <QTimer>
#include <QAction>

MUON_BACKEND_PLUGIN(SnapBackend)

SnapBackend::SnapBackend(QObject* parent)
    : AbstractResourcesBackend(parent)
    , m_updater(new StandardBackendUpdater(this))
    , m_reviews(new SnapReviewsBackend(this))
{
    connect(m_reviews, &SnapReviewsBackend::ratingsReady, this, &AbstractResourcesBackend::emitRatingsReady);
}

int SnapBackend::updatesCount() const
{
    return m_updater->updatesCount();
}

ResultsStream * SnapBackend::search(const AbstractResourcesBackend::Filters& filters)
{
    if (filters.state >= AbstractResource::Installed) {
        return populate(m_socket.snaps(), AbstractResource::Installed);
    } else {
        return populate(m_socket.find(filters.search), AbstractResource::None);
    }
    return new ResultsStream(QStringLiteral("Snap-void"), {});
}

ResultsStream * SnapBackend::findResourceByPackageName(const QString& search)
{
    return populate(m_socket.snapByName(search), AbstractResource::Installed);
}

ResultsStream* SnapBackend::populate(SnapJob* job, AbstractResource::State state)
{
    auto stream = new ResultsStream(QStringLiteral("Snap-populate"));

    connect(job, &SnapJob::finished, stream, [stream, this, state](SnapJob* job) {
        if (!job->isSuccessful()) {
            stream->deleteLater();
            return;
        }

        const auto snaps = job->result().toArray();

        QVector<AbstractResource*> ret;
        QSet<SnapResource*> resources;
        for(const auto& snap: snaps) {
            const auto snapObj = snap.toObject();
            const auto snapid = snapObj.value(QLatin1String("name")).toString();
            SnapResource* res = m_resources.value(snapid);
            if (!res) {
                res = new SnapResource(snapObj, state, this);
                Q_ASSERT(res->packageName() == snapid);
                resources += res;
            }
            ret += res;
        }

        if (!resources.isEmpty()) {
            foreach(SnapResource* res, resources)
                m_resources[res->packageName()] = res;
        }
        if (!ret.isEmpty())
            stream->resourcesFound(ret);
        stream->deleteLater();
    });
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

void SnapBackend::installApplication(AbstractResource* app, const AddonList& addons)
{
    Q_ASSERT(addons.isEmpty());
    installApplication(app);
}

void SnapBackend::installApplication(AbstractResource* _app)
{
	TransactionModel *transModel = TransactionModel::global();
    auto app = qobject_cast<SnapResource*>(_app);
    auto job = m_socket.snapAction(app->packageName(), SnapSocket::Install);
	transModel->addTransaction(new SnapTransaction(app, job, &m_socket, Transaction::InstallRole));
}

void SnapBackend::removeApplication(AbstractResource* _app)
{
	TransactionModel *transModel = TransactionModel::global();
    auto app = qobject_cast<SnapResource*>(_app);
    auto job = m_socket.snapAction(app->packageName(), SnapSocket::Remove);
	transModel->addTransaction(new SnapTransaction(app, job, &m_socket, Transaction::RemoveRole));
}

#include "SnapBackend.moc"
