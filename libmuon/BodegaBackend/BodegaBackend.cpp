/***************************************************************************
 *   Copyright © 2012 Aleix Pol Gonzalez <aleixpol@blue-systems.com>       *
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

#include "BodegaBackend.h"
#include "BodegaResource.h"
#include <Transaction/Transaction.h>
#include <bodega/session.h>
#include <bodega/listballotsjob.h>
#include <bodega/channelsjob.h>
#include <bodega/signonjob.h>
#include <bodega/installjob.h>
#include <bodega/uninstalljob.h>
#include <kwallet.h>
#include <KDebug>
#include <QDebug>

//copypaste ftw
QVariantHash retrieveCredentials()
{
    KWallet::Wallet *wallet = KWallet::Wallet::openWallet(KWallet::Wallet::NetworkWallet(), 0, KWallet::Wallet::Synchronous);
    if (wallet && wallet->isOpen() && wallet->setFolder("MakePlayLive")) {

        QMap<QString, QString> map;

        if (wallet->readMap("credentials", map) == 0) {
            QVariantHash hash;
            hash["username"] = map["username"];
            hash["password"] = map["password"];
            return hash;
        } else {
            kWarning() << "Unable to write credentials to wallet";
        }
    } else {
        kWarning() << "Unable to open wallet";
    }

    return QVariantHash();
}

BodegaBackend::BodegaBackend(const QString& channel, const QString& iconName, QObject* parent)
    : AbstractResourcesBackend(parent)
    , m_channel(channel)
    , m_icon(iconName)
{
    m_session = new Bodega::Session(this);
    QVariantHash credentials = retrieveCredentials();
    m_session->setUserName(credentials["username"].toString());
    m_session->setPassword(credentials["password"].toString());
    m_session->setBaseUrl(QUrl("http://addons.makeplaylive.com:3000"));
    m_session->setStoreId("VIVALDI-1");
    connect(m_session, SIGNAL(authenticated(bool)), SLOT(resetResources()));

    m_session->signOn();
}

BodegaBackend::~BodegaBackend()
{}

void BodegaBackend::resetResources()
{
    if(!m_session->isAuthenticated())
        return;
    
    Bodega::ChannelsJob* job = m_session->channels();
    connect(job, SIGNAL(jobFinished(Bodega::NetworkJob*)), SLOT(channelsRetrieved(Bodega::NetworkJob*)));
}

void BodegaBackend::channelsRetrieved(Bodega::NetworkJob* job)
{
    qDebug() << "channels received";
    Bodega::ChannelsJob* ballotsJob = qobject_cast<Bodega::ChannelsJob*>(job);
    QList<Bodega::ChannelInfo> channels = ballotsJob->channels();
    
    foreach(const Bodega::ChannelInfo& c, channels) {
        if(c.name == m_channel) {
            Bodega::ChannelsJob* wallpapersChannel = m_session->channels(c.id);
            connect(wallpapersChannel, SIGNAL(jobFinished(Bodega::NetworkJob*)), SLOT(dataReceived(Bodega::NetworkJob*)));
        }
    }
}

void BodegaBackend::dataReceived(Bodega::NetworkJob* job)
{
    Bodega::ChannelsJob* cjob = qobject_cast<Bodega::ChannelsJob*>(job);
    QList<Bodega::AssetInfo> assets = cjob->assets();
    
    foreach(const Bodega::AssetInfo& a, assets) {
        m_resourcesByName.insert(a.name, new BodegaResource(a, this));
    }
    emit backendReady();
}

QVector<AbstractResource*> BodegaBackend::allResources() const
{
    return m_resourcesByName.values().toVector();
}

QStringList BodegaBackend::searchPackageName(const QString& searchText){
    QStringList ret;
    foreach(AbstractResource* r, m_resourcesByName) {
        if(r->name().contains(searchText) || r->comment().contains(searchText))
            ret += r->packageName();
    }
    return ret;
}

AbstractResource* BodegaBackend::resourceByPackageName(const QString& name) const
{
    return m_resourcesByName.value(name);
}

AbstractBackendUpdater* BodegaBackend::backendUpdater() const
{ return 0; }

void BodegaBackend::installApplication(AbstractResource* app, const QHash< QString, bool >& addons)
{
    BodegaResource* res = qobject_cast<BodegaResource*>(app);
    Bodega::InstallJob* job = m_session->install(m_session->assetOperations(res->assetId()));
    
    Transaction* t = new Transaction(res, InstallApp);
    emit transactionAdded(t);
    t->setProperty("job", qVariantFromValue<QObject*>(job));
    m_transactions.append(t);
    connect(job, SIGNAL(jobFinished(Bodega::NetworkJob*)), SLOT(removeTransaction(Bodega::NetworkJob*)));
}

void BodegaBackend::removeApplication(AbstractResource* app)
{
    BodegaResource* res = qobject_cast<BodegaResource*>(app);
    Bodega::UninstallJob* job = m_session->uninstall(m_session->assetOperations(res->assetId()));
    
    Transaction* t = new Transaction(res, RemoveApp);
    emit transactionAdded(t);
    t->setProperty("job", qVariantFromValue<QObject*>(job));
    m_transactions.append(t);
    connect(job, SIGNAL(jobFinished(Bodega::UninstallJob*)), SLOT(removeTransaction(Bodega::UninstallJob*)));
}

void BodegaBackend::removeTransaction(Bodega::NetworkJob* job) { removeTransactionGeneric(job); }

void BodegaBackend::removeTransaction(Bodega::UninstallJob* job) { removeTransactionGeneric(job); }

void BodegaBackend::removeTransactionGeneric(QObject* job)
{
    foreach(Transaction* t, m_transactions) {
        if(t->property("job").value<QObject*>() == job) {
            emit transactionRemoved(t);
            m_transactions.removeAll(t);
            break;
        }
    }
}

void BodegaBackend::cancelTransaction(AbstractResource* app)
{
    foreach(Transaction* t, m_transactions) {
        if(t->resource() == app) {
            Bodega::NetworkJob* job = qobject_cast<Bodega::NetworkJob*>(t->property("job").value<QObject*>());
            job->reply()->abort();
            m_transactions.removeAll(t);
            emit transactionCancelled(t);
            delete t;
            break;
        }
    }
}

QPair< TransactionStateTransition, Transaction* > BodegaBackend::currentTransactionState() const {
    Transaction* t = 0;
    if(!m_transactions.isEmpty())
        t = m_transactions.first();
    return qMakePair<TransactionStateTransition, Transaction*>(FinishedDownloading, t);
}

QList< Transaction* > BodegaBackend::transactions() const { return m_transactions; }

AbstractReviewsBackend* BodegaBackend::reviewsBackend() const { return 0; }

int BodegaBackend::updatesCount() const { return upgradeablePackages().count(); }

QList<AbstractResource*> BodegaBackend::upgradeablePackages()
{
    QList<AbstractResource*> ret;
    foreach(AbstractResource* res, m_resourcesByName) {
        if(res->state()==AbstractResource::Upgradeable)
            ret += res;
    }
    return ret;
}
