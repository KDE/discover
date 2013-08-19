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

// Qt includes
#include <QDebug>
#include <QFileInfo>

// Attica includes
#include <attica/content.h>
#include <attica/providermanager.h>

// KDE includes
#include <knewstuff3/downloadmanager.h>
#include <KStandardDirs>
#include <KConfigGroup>
#include <KDebug>
#include <KConfig>
#include <KPluginFactory>
#include <KLocalizedString>
#include <KAboutData>

// Libmuon includes
#include "Transaction/Transaction.h"
#include "Transaction/TransactionModel.h"

// Own includes
#include "KNSBackend.h"
#include "KNSResource.h"
#include "KNSReviews.h"
#include <resources/StandardBackendUpdater.h>

K_PLUGIN_FACTORY(MuonKNSBackendFactory, registerPlugin<KNSBackend>(); )
K_EXPORT_PLUGIN(MuonKNSBackendFactory(KAboutData("muon-knsbackend","muon-knsbackend",ki18n("KNewStuff Backend"),"0.1",ki18n("Install KNewStuff data in your system"), KAboutData::License_GPL)))

QSharedPointer<Attica::ProviderManager> KNSBackend::m_atticaManager;

void KNSBackend::initManager(KConfigGroup& group)
{
    if(!m_atticaManager) {
        m_atticaManager = QSharedPointer<Attica::ProviderManager>(new Attica::ProviderManager);
        QString entry = group.readEntry("ProvidersUrl", QString());
        if(!m_atticaManager->defaultProviderFiles().contains(entry))
            m_atticaManager->addProviderFileToDefaultProviders(entry);
        m_atticaManager->loadDefaultProviders();
    }
}

KNSBackend::KNSBackend(QObject* parent, const QVariantList& args)
    : AbstractResourcesBackend(parent)
    , m_isValid(true)
    , m_page(0)
    , m_reviews(new KNSReviews(this))
    , m_fetching(true)
    , m_updater(new StandardBackendUpdater(this))
{
    const QVariantMap info = args.first().toMap();
    
    m_iconName = info.value("Icon").toString();
    m_name = KStandardDirs::locate("config", info.value("X-Muon-Arguments").toString());
    Q_ASSERT(!m_name.isEmpty());
    KConfig conf(m_name);
    KConfigGroup group;

    if (conf.hasGroup("KNewStuff3"))
        group = conf.group("KNewStuff3");

    if (!group.isValid()) {
        m_isValid = false;
        kWarning() << "Config group not found! Check your KNS3 installation.";
        return;
    }

    QStringList cats = group.readEntry("Categories", QStringList());
    initManager(group);
    connect(m_atticaManager.data(), SIGNAL(defaultProvidersLoaded()), SLOT(startFetchingCategories()));
    
    foreach(const QString& c, cats) {
        m_categories.insert(c, Attica::Category());
    }
    
    m_manager = new KNS3::DownloadManager(m_name, this);
    connect(m_manager, SIGNAL(searchResult(KNS3::Entry::List)), SLOT(receivedEntries(KNS3::Entry::List)));
    connect(m_manager, SIGNAL(entryStatusChanged(KNS3::Entry)), SLOT(statusChanged(KNS3::Entry)));
    
    startFetchingCategories();
}

KNSBackend::~KNSBackend()
{}

void KNSBackend::setFetching(bool f)
{
    if(m_fetching!=f) {
        m_fetching = f;
        emit fetchingChanged();
    }
}

bool KNSBackend::isValid() const
{
    return m_isValid;
}

void KNSBackend::startFetchingCategories()
{
    if (m_atticaManager->providers().isEmpty())
        return;

    m_provider = m_atticaManager->providers().first();

    Attica::ListJob<Attica::Category>* job = m_provider.requestCategories();
    connect(job, SIGNAL(finished(Attica::BaseJob*)), SLOT(categoriesLoaded(Attica::BaseJob*)));
    job->start();
}

void KNSBackend::categoriesLoaded(Attica::BaseJob* job)
{
    if(job->metadata().error() != Attica::Metadata::NoError) {
        kDebug() << "Network error";
        return;
    }
    Attica::ListJob<Attica::Category>* j = static_cast<Attica::ListJob<Attica::Category>*>(job);
    Attica::Category::List categoryList = j->itemList();

    foreach(const Attica::Category& category, categoryList) {
        if (m_categories.contains(category.name())) {
            kDebug() << "Adding category: " << category.name();
            m_categories[category.name()] = category;
        }
    }
    
    Attica::ListJob<Attica::Content>* jj =
        m_provider.searchContents(m_categories.values(), QString(), Attica::Provider::Alphabetical, m_page, 100);
    connect(jj, SIGNAL(finished(Attica::BaseJob*)), SLOT(receivedContents(Attica::BaseJob*)));
    jj->start();
}

void KNSBackend::receivedContents(Attica::BaseJob* job)
{
    if(job->metadata().error() != Attica::Metadata::NoError) {
        kDebug() << "Network error";
        return;
    }
    Attica::ListJob<Attica::Content>* listJob = static_cast<Attica::ListJob<Attica::Content>*>(job);
    Attica::Content::List contents = listJob->itemList();
    
    if(contents.isEmpty()) {
        setFetching(false);
        m_page = 0;
        m_manager->search();
        return;
    }
    QString filename = QFileInfo(m_name).fileName();
    foreach(const Attica::Content& c, contents) {
        KNSResource* r = new KNSResource(c, filename, m_iconName, this);
        m_resourcesByName.insert(c.id(), r);
        connect(r, SIGNAL(stateChanged()), SIGNAL(updatesCountChanged()));
    }
    m_page++;
    Attica::ListJob<Attica::Content>* jj =
        m_provider.searchContents(m_categories.values(), QString(), Attica::Provider::Alphabetical, m_page, 100);
    connect(jj, SIGNAL(finished(Attica::BaseJob*)), SLOT(receivedContents(Attica::BaseJob*)));
    jj->start();
}

void KNSBackend::receivedEntries(const KNS3::Entry::List& entries)
{
    if(entries.isEmpty()) {
        setFetching(false);
        return;
    }
    
    foreach(const KNS3::Entry& entry, entries) {
        KNSResource* r = qobject_cast<KNSResource*>(m_resourcesByName.value(entry.id()));
        r->setEntry(entry);
    }
    ++m_page;
    m_manager->search(m_page);
}

void KNSBackend::statusChanged(const KNS3::Entry& entry)
{
    KNSResource* r = qobject_cast<KNSResource*>(m_resourcesByName.value(entry.id()));
    if(r)
        r->setEntry(entry);
    else
        kWarning() << "unknown entry changed" << entry.id() << entry.name();
}

void KNSBackend::cancelTransaction(AbstractResource* app)
{
    Q_UNUSED(app)

    qWarning("KNS transaction cancelling unsupported");
}

void KNSBackend::removeApplication(AbstractResource* app)
{
    Transaction* t = new Transaction(this, app, Transaction::RemoveRole);
    TransactionModel *transModel = TransactionModel::global();
    transModel->addTransaction(t);
    KNSResource* r = qobject_cast<KNSResource*>(app);
    Q_ASSERT(r->entry());
    m_manager->uninstallEntry(*r->entry());
    transModel->removeTransaction(t);
}

void KNSBackend::installApplication(AbstractResource* app)
{
    Transaction* t = new Transaction(this, app, Transaction::InstallRole);
    TransactionModel *transModel = TransactionModel::global();
    transModel->addTransaction(t);
    KNSResource* r = qobject_cast<KNSResource*>(app);
    Q_ASSERT(r->entry());
    m_manager->installEntry(*r->entry());
    transModel->removeTransaction(t);
}

void KNSBackend::installApplication(AbstractResource* app, AddonList)
{
    installApplication(app);
}

AbstractResource* KNSBackend::resourceByPackageName(const QString& name) const
{
    return m_resourcesByName[name];
}

int KNSBackend::updatesCount() const
{
    int ret = 0;
    foreach(AbstractResource* r, m_resourcesByName) {
        if(r->state()==AbstractResource::Upgradeable)
            ++ret;
    }
    return ret;
}

QList<AbstractResource*> KNSBackend::upgradeablePackages() const
{
    QList<AbstractResource*> ret;
    foreach(AbstractResource* r, m_resourcesByName) {
        if(r->state()==AbstractResource::Upgradeable)
            ret+=r;
    }
    return ret;
}

AbstractReviewsBackend* KNSBackend::reviewsBackend() const
{
    return m_reviews;
}

QList<AbstractResource*> KNSBackend::searchPackageName(const QString& searchText)
{
    QList<AbstractResource*> ret;
    foreach(AbstractResource* r, m_resourcesByName) {
        if(r->name().contains(searchText, Qt::CaseInsensitive) || r->comment().contains(searchText, Qt::CaseInsensitive))
            ret += r;
    }
    return ret;
}

QVector< AbstractResource* > KNSBackend::allResources() const
{
    return m_resourcesByName.values().toVector();
}

bool KNSBackend::isFetching() const
{
    return m_fetching;
}

AbstractBackendUpdater* KNSBackend::backendUpdater() const
{
    return m_updater;
}
