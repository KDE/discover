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
    , m_fetching(false)
    , m_startElements(320)
{}

void DummyBackend::setMetaData(const QString& path)
{
    Q_ASSERT(!path.isEmpty());
    KSharedConfig::Ptr cfg = KSharedConfig::openConfig(path);
    KConfigGroup metadata = cfg->group(QStringLiteral("Desktop Entry"));

    populate(metadata.readEntry("Name", QString()));
    m_reviews->initialize();

    QAction* updateAction = new QAction(this);
    updateAction->setIcon(QIcon::fromTheme("system-software-update"));
    updateAction->setText(i18nc("@action Checks the Internet for updates", "Check for Updates"));
    updateAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_R));
    connect(updateAction, SIGNAL(triggered()), SLOT(checkForUpdates()));

    QAction* randomAction = new QAction(this);
    randomAction->setIcon(QIcon::fromTheme("kalarm"));
    randomAction->setText(QStringLiteral("test bla bla"));
    randomAction->setShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_T));
    randomAction->setPriority(QAction::LowPriority);
    connect(randomAction, &QAction::triggered, this, [](){ qDebug() << "random action triggered"; });
    m_messageActions = QList<QAction*>() << updateAction << randomAction;
    m_updater->setMessageActions(m_messageActions);

    SourcesModel::global()->addSourcesBackend(new DummySourcesBackend(this));
}

void DummyBackend::populate(const QString& n)
{
    int start = m_resources.count();
    for(int i=start; i<start+m_startElements; i++) {
        QString name = n+' '+QString::number(i);
        DummyResource* res = new DummyResource(name, false, this);
        res->setState(AbstractResource::State(1+(i%3)));
        m_resources.insert(name, res);
        connect(res, SIGNAL(stateChanged()), SIGNAL(updatesCountChanged()));
    }

    for(int i=start; i<start+m_startElements; i++) {
        QString name = "techie"+QString::number(i);
        DummyResource* res = new DummyResource(name, true, this);
        res->setState(AbstractResource::State(1+(i%3)));
        m_resources.insert(name, res);
        connect(res, SIGNAL(stateChanged()), SIGNAL(updatesCountChanged()));
    }
}

void DummyBackend::toggleFetching()
{
    m_fetching = !m_fetching;
    qDebug() << "fetching..." << m_fetching;
    emit fetchingChanged();
    if (!m_fetching)
        m_reviews->initialize();
}

QVector<AbstractResource*> DummyBackend::allResources() const
{
    Q_ASSERT(!m_fetching);
    QVector<AbstractResource*> ret;
    ret.reserve(m_resources.size());
    foreach(AbstractResource* res, m_resources) {
        ret += res;
    }
    return ret;
}

int DummyBackend::updatesCount() const
{
    return upgradeablePackages().count();
}

QList<AbstractResource*> DummyBackend::upgradeablePackages() const
{
    QList<AbstractResource*> updates;
    foreach(AbstractResource* res, m_resources) {
        if(res->state()==AbstractResource::Upgradeable)
            updates += res;
    }
    return updates;
}

AbstractResource* DummyBackend::resourceByPackageName(const QString& name) const
{
    return m_resources.value(name);
}

QList<AbstractResource*> DummyBackend::searchPackageName(const QString& searchText)
{
    QList<AbstractResource*> ret;
    foreach(AbstractResource* r, m_resources) {
        if(r->name().contains(searchText, Qt::CaseInsensitive) || r->comment().contains(searchText, Qt::CaseInsensitive))
            ret += r;
    }
    return ret;
}

AbstractBackendUpdater* DummyBackend::backendUpdater() const
{
    return m_updater;
}

AbstractReviewsBackend* DummyBackend::reviewsBackend() const
{
    return m_reviews;
}

void DummyBackend::installApplication(AbstractResource* app, AddonList addons)
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

void DummyBackend::cancelTransaction(AbstractResource*)
{}

void DummyBackend::checkForUpdates()
{
    if(m_fetching)
        return;
    toggleFetching();
    populate("Moar");
    QTimer::singleShot(500, this, SLOT(toggleFetching()));
}

#include "DummyBackend.moc"
