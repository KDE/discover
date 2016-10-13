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

    populate(m_socket.snaps());
}

QVector<AbstractResource*> SnapBackend::allResources() const
{
    QVector<AbstractResource*> ret;
    ret.reserve(m_resources.size());
    foreach(AbstractResource* res, m_resources) {
        ret += res;
    }
    return ret;
}

int SnapBackend::updatesCount() const
{
    return m_updater->updatesCount();
}

AbstractResource* SnapBackend::resourceByPackageName(const QString& name) const
{
    return m_resources.value(name);
}

QList<AbstractResource*> SnapBackend::searchPackageName(const QString& searchText)
{
    return populate(m_socket.find(searchText));
}

QList<AbstractResource*> SnapBackend::populate(SnapJob* job)
{
    if (!job->exec()) {
        qWarning() << "job failed" << job;
        return {};
    }

    const auto snaps = job->result().toArray();

    QList<AbstractResource*> ret;
    QSet<SnapResource*> resources;
    for(const auto& snap: snaps) {
        const auto snapObj = snap.toObject();
        const auto snapid = snapObj.value(QLatin1String("id")).toString();
        SnapResource* res = m_resources.value(snapid);
        if (!res) {
            res = new SnapResource(snapObj, this);
            Q_ASSERT(res->packageName() == snapid);
            resources += res;
        }
        ret += res;
    }

    if (!resources.isEmpty()) {
        setFetching(true);
        foreach(SnapResource* res, resources)
            m_resources[res->packageName()] = res;
        setFetching(false);
    }
    return ret;
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
//     TransactionModel *transModel = TransactionModel::global();
//     transModel->addTransaction(new SnapTransaction(qobject_cast<SnapResource*>(app), addons, Transaction::InstallRole));
}

void SnapBackend::installApplication(AbstractResource* app)
{
// 	TransactionModel *transModel = TransactionModel::global();
// 	transModel->addTransaction(new SnapTransaction(qobject_cast<SnapResource*>(app), Transaction::InstallRole));
}

void SnapBackend::removeApplication(AbstractResource* app)
{
// 	TransactionModel *transModel = TransactionModel::global();
// 	transModel->addTransaction(new SnapTransaction(qobject_cast<SnapResource*>(app), Transaction::RemoveRole));
}

#include "SnapBackend.moc"
