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

#include "DummyBackend.h"
#include "DummyResource.h"
#include "DummyReviewsBackend.h"
#include "DummyTransaction.h"
#include "DummySourcesBackend.h"
#include <resources/StandardBackendUpdater.h>
#include <resources/SourcesModel.h>
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

DISCOVER_BACKEND_PLUGIN(DummyBackend)

DummyBackend::DummyBackend(QObject* parent)
    : AbstractResourcesBackend(parent)
    , m_updater(new StandardBackendUpdater(this))
    , m_reviews(new DummyReviewsBackend(this))
    , m_fetching(true)
    , m_startElements(120)
{
    QTimer::singleShot(500, this, &DummyBackend::toggleFetching);
    connect(m_reviews, &DummyReviewsBackend::ratingsReady, this, &AbstractResourcesBackend::emitRatingsReady);
    connect(m_updater, &StandardBackendUpdater::updatesCountChanged, this, &DummyBackend::updatesCountChanged);

    populate(QStringLiteral("Dummy"));
    if (!m_fetching)
        m_reviews->initialize();

    SourcesModel::global()->addSourcesBackend(new DummySourcesBackend(this));
}

void DummyBackend::populate(const QString& n)
{
    const int start = m_resources.count();
    for(int i=start; i<start+m_startElements; i++) {
        const QString name = n+QLatin1Char(' ')+QString::number(i);
        DummyResource* res = new DummyResource(name, false, this);
        res->setSize(100+(m_startElements-i));
        res->setState(AbstractResource::State(1+(i%3)));
        m_resources.insert(name.toLower(), res);
        connect(res, &DummyResource::stateChanged, this, &DummyBackend::updatesCountChanged);
    }

    for(int i=start; i<start+m_startElements; i++) {
        const QString name = QStringLiteral("techie")+QString::number(i);
        DummyResource* res = new DummyResource(name, true, this);
        res->setState(AbstractResource::State(1+(i%3)));
        res->setSize(300+(m_startElements-i));
        m_resources.insert(name, res);
        connect(res, &DummyResource::stateChanged, this, &DummyBackend::updatesCountChanged);
    }
}

void DummyBackend::toggleFetching()
{
    m_fetching = !m_fetching;
//     qDebug() << "fetching..." << m_fetching;
    emit fetchingChanged();
    if (!m_fetching)
        m_reviews->initialize();
}

int DummyBackend::updatesCount() const
{
    return m_updater->updatesCount();
}

ResultsStream* DummyBackend::search(const AbstractResourcesBackend::Filters& filter)
{
    QVector<AbstractResource*> ret;
    if (!filter.resourceUrl.isEmpty() && filter.resourceUrl.scheme() == QLatin1String("dummy"))
        return findResourceByPackageName(filter.resourceUrl);
    else foreach(AbstractResource* r, m_resources) {
        if(r->name().contains(filter.search, Qt::CaseInsensitive) || r->comment().contains(filter.search, Qt::CaseInsensitive))
            ret += r;
    }
    return new ResultsStream(QStringLiteral("DummyStream"), ret);
}

ResultsStream * DummyBackend::findResourceByPackageName(const QUrl& search)
{
    auto res = search.scheme() == QLatin1String("dummy") ? m_resources.value(search.host().replace(QLatin1Char('.'), QLatin1Char(' '))) : nullptr;
    if (!res) {
        return new ResultsStream(QStringLiteral("DummyStream"), {});
    } else
        return new ResultsStream(QStringLiteral("DummyStream"), { res });
}

AbstractBackendUpdater* DummyBackend::backendUpdater() const
{
    return m_updater;
}

AbstractReviewsBackend* DummyBackend::reviewsBackend() const
{
    return m_reviews;
}

Transaction* DummyBackend::installApplication(AbstractResource* app, const AddonList& addons)
{
    return new DummyTransaction(qobject_cast<DummyResource*>(app), addons, Transaction::InstallRole);
}

Transaction* DummyBackend::installApplication(AbstractResource* app)
{
	return new DummyTransaction(qobject_cast<DummyResource*>(app), Transaction::InstallRole);
}

Transaction* DummyBackend::removeApplication(AbstractResource* app)
{
	return new DummyTransaction(qobject_cast<DummyResource*>(app), Transaction::RemoveRole);
}

void DummyBackend::checkForUpdates()
{
    if(m_fetching)
        return;
    toggleFetching();
    populate(QStringLiteral("Moar"));
    QTimer::singleShot(500, this, &DummyBackend::toggleFetching);
    qDebug() << "DummyBackend::checkForUpdates";
}

AbstractResource * DummyBackend::resourceForFile(const QUrl& path)
{
    DummyResource* res = new DummyResource(path.fileName(), true, this);
    res->setSize(666);
    res->setState(AbstractResource::None);
    m_resources.insert(res->packageName(), res);
    connect(res, &DummyResource::stateChanged, this, &DummyBackend::updatesCountChanged);
    return res;
}

QString DummyBackend::displayName() const
{
    return QStringLiteral("Dummy");
}

bool DummyBackend::hasApplications() const
{
    return true;
}

#include "DummyBackend.moc"
