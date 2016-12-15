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

MUON_BACKEND_PLUGIN(DummyBackend)

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

    QAction* updateAction = new QAction(this);
    updateAction->setIcon(QIcon::fromTheme(QStringLiteral("system-software-update")));
    updateAction->setText(i18nc("@action Checks the Internet for updates", "Check for Updates"));
    updateAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_R));
    connect(updateAction, &QAction::triggered, this, &DummyBackend::checkForUpdates);

    QAction* randomAction = new QAction(this);
    randomAction->setIcon(QIcon::fromTheme(QStringLiteral("kalarm")));
    randomAction->setText(QStringLiteral("test bla bla"));
    randomAction->setShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_T));
    randomAction->setPriority(QAction::LowPriority);
    connect(randomAction, &QAction::triggered, this, [](){ qDebug() << "random action triggered"; });

//     QAction* importantAction = new QAction(this);
//     importantAction->setIcon(QIcon::fromTheme(QStringLiteral("kalarm"));
//     importantAction->setText(QStringLiteral("Amaze!"));
//     importantAction->setWhatsThis(QStringLiteral("Wo Wo I'm so important"));
//     importantAction->setPriority(QAction::HighPriority);
//     connect(importantAction, &QAction::triggered, this, [importantAction](){
//         importantAction->setEnabled(false);
//         qDebug() << "important action triggered";
//     });

    m_messageActions = QList<QAction*>() << updateAction << randomAction /*<< importantAction*/;

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
        m_resources.insert(name, res);
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
    foreach(AbstractResource* r, m_resources) {
        if(r->name().contains(filter.search, Qt::CaseInsensitive) || r->comment().contains(filter.search, Qt::CaseInsensitive))
            ret += r;
    }
    return new ResultsStream(QStringLiteral("DummyStream"), ret);
}

ResultsStream * DummyBackend::findResourceByPackageName(const QString& search)
{
    auto res = m_resources.value(search);
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

void DummyBackend::installApplication(AbstractResource* app, const AddonList& addons)
{
    TransactionModel *transModel = TransactionModel::global();
    transModel->addTransaction(new DummyTransaction(qobject_cast<DummyResource*>(app), addons, Transaction::InstallRole));
}

void DummyBackend::installApplication(AbstractResource* app)
{
	TransactionModel *transModel = TransactionModel::global();
	transModel->addTransaction(new DummyTransaction(qobject_cast<DummyResource*>(app), Transaction::InstallRole));
}

void DummyBackend::removeApplication(AbstractResource* app)
{
	TransactionModel *transModel = TransactionModel::global();
	transModel->addTransaction(new DummyTransaction(qobject_cast<DummyResource*>(app), Transaction::RemoveRole));
}

void DummyBackend::checkForUpdates()
{
    if(m_fetching)
        return;
    toggleFetching();
    populate(QStringLiteral("Moar"));
    QTimer::singleShot(500, this, &DummyBackend::toggleFetching);
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

#include "DummyBackend.moc"
