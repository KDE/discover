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
#include <Transaction/TransactionModel.h>
#include <bodega/session.h>
#include <bodega/channelsjob.h>
#include <bodega/signonjob.h>
#include <bodega/installjob.h>
#include <bodega/uninstalljob.h>
#include <kwallet.h>
#include <KDebug>
#include <KAboutData>
#include <KPluginFactory>
#include <KPasswordDialog>
#include <QDebug>

K_PLUGIN_FACTORY(MuonBodegaBackendFactory, registerPlugin<BodegaBackend>(); )
K_EXPORT_PLUGIN(MuonBodegaBackendFactory(KAboutData("muon-bodegabackend","muon-bodegabackend",ki18n("Bodega Backend"),"0.1",ki18n("Install Bodega data in your system"), KAboutData::License_GPL)))

QMap<QString,QString> retrieveCredentials(const QString& folderName)
{
    QMap<QString,QString> ret;
    KWallet::Wallet *wallet = KWallet::Wallet::openWallet(KWallet::Wallet::NetworkWallet(), 0, KWallet::Wallet::Synchronous);
    if (wallet && wallet->isOpen()) {
        bool folderExists = wallet->setFolder(folderName);
        if (folderExists) {
            QMap<QString, QString> map;

            if (wallet->readMap("credentials", map) == 0 && map.contains("username") && map.contains("password")) {
                ret["username"] = map.value("username");
                ret["password"] = map.value("password");
            } else {
                kWarning() << "Unable to read credentials from wallet";
            }
        }

        if (ret.isEmpty()) {
            QPointer<KPasswordDialog> dialog(new KPasswordDialog(0, KPasswordDialog::ShowKeepPassword|KPasswordDialog::ShowUsernameLine));
            dialog->setPrompt(i18n("Enter %1 credentials", folderName));
            dialog->exec();
            ret["username"] = dialog->username();
            ret["password"] = dialog->password();
            if(dialog->keepPassword()) {
                folderExists = (folderExists || wallet->createFolder(folderName)) &&
                               wallet->setFolder(folderName) &&
                               wallet->writeMap("credentials", ret)==0;
            }
            delete dialog;
        }
    } else {
        kWarning() << "Unable to open wallet";
    }

    return ret;
}

BodegaBackend::BodegaBackend(QObject* parent, const QVariantList& args)
    : AbstractResourcesBackend(parent)
    , m_fetching(false)
{
    const QVariantMap info = args.first().toMap();
    
    m_icon = info.value("Icon").toString();
    QUrl url = info.value("X-Muon-Arguments").toString();
    m_channel = url.queryItemValue("channel");
    QString storeId = url.queryItemValue("storeId");
    QMap<QString,QString> credentials = retrieveCredentials(url.queryItemValue("credentials"));
    url.setQueryItems(QList<QPair<QString,QString> >());
    
    m_session = new Bodega::Session(this);
    m_session->setUserName(credentials["username"]);
    m_session->setPassword(credentials["password"]);
    m_session->setBaseUrl(url);
    m_session->setStoreId(storeId);
    connect(m_session, SIGNAL(authenticated(bool)), SLOT(resetResources()));
    m_session->signOn();
}

BodegaBackend::~BodegaBackend()
{}

void BodegaBackend::resetResources()
{
    if(!m_session->isAuthenticated()) {
        qDebug() << "not authenticated!" << m_session->userName();
        return;
    }
    
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

    setFetching(true);
    foreach(const Bodega::AssetInfo& a, assets) {
        m_resourcesByName.insert(a.name, new BodegaResource(a, this));
    }
    setFetching(false);
}

QVector<AbstractResource*> BodegaBackend::allResources() const
{
    return m_resourcesByName.values().toVector();
}

QList<AbstractResource*> BodegaBackend::searchPackageName(const QString& searchText){
    QList<AbstractResource*> ret;
    foreach(AbstractResource* r, m_resourcesByName) {
        if(r->name().contains(searchText, Qt::CaseInsensitive) || r->comment().contains(searchText, Qt::CaseInsensitive))
            ret += r;
    }
    return ret;
}

AbstractResource* BodegaBackend::resourceByPackageName(const QString& name) const
{
    return m_resourcesByName.value(name);
}

AbstractBackendUpdater* BodegaBackend::backendUpdater() const
{ return 0; }

void BodegaBackend::installApplication(AbstractResource* app, AddonList addons)
{
    Q_ASSERT(m_transactions.count()==0);
    Q_ASSERT(addons.isEmpty());
    BodegaResource* res = qobject_cast<BodegaResource*>(app);
    Transaction* t = new Transaction(this, res, Transaction::InstallRole);
    TransactionModel *transModel = TransactionModel::global();
    transModel->addTransaction(t);
    m_transactions.append(t);
    t->setStatus(Transaction::CommittingStatus);
    
    Bodega::InstallJob* job = m_session->install(res->assetOperations());
    t->setProperty("job", qVariantFromValue<QObject*>(job));
    connect(job, SIGNAL(jobFinished(Bodega::NetworkJob*)), SLOT(removeTransaction(Bodega::NetworkJob*)));
}

void BodegaBackend::removeApplication(AbstractResource* app)
{
    Q_ASSERT(m_transactions.count()==0);
    BodegaResource* res = qobject_cast<BodegaResource*>(app);
    Transaction* t = new Transaction(this, res, Transaction::RemoveRole);
    TransactionModel *transModel = TransactionModel::global();
    transModel->addTransaction(t);
    m_transactions.append(t);
    t->setStatus(Transaction::CommittingStatus);
    
    Bodega::UninstallJob* job = m_session->uninstall(res->assetOperations());
    t->setProperty("job", qVariantFromValue<QObject*>(job));
    connect(job, SIGNAL(jobFinished(Bodega::UninstallJob*)), SLOT(removeTransaction(Bodega::UninstallJob*)));
}

void BodegaBackend::removeTransaction(Bodega::NetworkJob* job) { removeTransactionGeneric(job); }
void BodegaBackend::removeTransaction(Bodega::UninstallJob* job) { removeTransactionGeneric(job); }

void BodegaBackend::removeTransactionGeneric(QObject* job)
{
    Q_ASSERT(m_transactions.count()==1);
    if(job->property("failed").toBool()) {
        qDebug() << "job failed" << job->metaObject()->className() << job->property("error").value<Bodega::Error>().title();
    }
    
    qDebug() << "finished" << job;
    TransactionModel *transModel = TransactionModel::global();
    foreach(Transaction* t, m_transactions) {
        if(t->property("job").value<QObject*>() == job) {
            t->setStatus(Transaction::DoneStatus);
            transModel->removeTransaction(t);
            m_transactions.removeAll(t);
            delete t;
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
            TransactionModel::global()->cancelTransaction(t);
            delete t;
            break;
        }
    }
}

AbstractReviewsBackend* BodegaBackend::reviewsBackend() const { return nullptr; }

int BodegaBackend::updatesCount() const { return upgradeablePackages().count(); }

QList<AbstractResource*> BodegaBackend::upgradeablePackages() const
{
    QList<AbstractResource*> ret;
    foreach(AbstractResource* res, m_resourcesByName.values()) {
        if(res->state()==AbstractResource::Upgradeable)
            ret += res;
    }
    return ret;
}

void BodegaBackend::setFetching(bool f)
{
    if(f!=m_fetching) {
        m_fetching = f;
        emit fetchingChanged();
    }
}
