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

#include "KNSBackend.h"
#include "KNSResource.h"
#include "KNSReviews.h"
#include <Transaction/Transaction.h>
#include <knewstuff3/downloadmanager.h>
#include <QDebug>
#include <QFileInfo>
#include <attica/content.h>
#include <attica/providermanager.h>
#include <KStandardDirs>
#include <KConfigGroup>
#include <KDebug>
#include <kconfig.h>

KNSBackend::KNSBackend(const QString& configName, QObject* parent)
    : AbstractResourcesBackend(parent)
    , m_reviews(new KNSReviews(this))
    , m_fetching(true)
{
    m_name = KStandardDirs::locate("config", configName);
    KConfig conf(m_name);
    KConfigGroup group;
    if (conf.hasGroup("KNewStuff3")) {
        group = conf.group("KNewStuff3");
    }
    QStringList cats = group.readEntry("Categories", QStringList());
    m_atticaManager = new Attica::ProviderManager;
    m_atticaManager->addProviderFileToDefaultProviders(group.readEntry("ProvidersUrl", QString()));
    m_atticaManager->loadDefaultProviders();
    connect(m_atticaManager, SIGNAL(defaultProvidersLoaded()), SLOT(startFetchingCategories()));
    m_page = 0;
    
    foreach(const QString& c, cats) {
        m_categories.insert(c, Attica::Category());
    }
    
    m_manager = new KNS3::DownloadManager(m_name, this);
    connect(m_manager, SIGNAL(searchResult(KNS3::Entry::List)), SLOT(receivedEntries(KNS3::Entry::List)));
    
    connect(m_reviews, SIGNAL(ratingsReady()), SLOT(fillEntries()));
}

KNSBackend::~KNSBackend()
{
    delete m_atticaManager;
}

void KNSBackend::startFetchingCategories()
{
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
        m_fetching = false;
        emit backendReady();
        m_page = 0;
        m_manager->search();
        return;
    }
    foreach(const Attica::Content& c, contents) {
        m_resourcesByName.insert(c.id(), new KNSResource(c, QFileInfo(m_name).fileName(), this));
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
        return;
    }
    
    foreach(const KNS3::Entry& entry, entries) {
        KNSResource* r = qobject_cast<KNSResource*>(m_resourcesByName.value(entry.id()));
        r->setEntry(entry);
    }
    ++m_page;
    m_manager->search(m_page);
}

void KNSBackend::cancelTransaction(AbstractResource* app)
{
    qWarning("KNS transaction cancelling unsupported");
}

void KNSBackend::removeApplication(AbstractResource* app)
{
    Transaction* t = new Transaction(app, RemoveApp);
    emit transactionAdded(t);
    KNSResource* r = qobject_cast<KNSResource*>(app);
    Q_ASSERT(r->entry());
    m_manager->uninstallEntry(*r->entry());
    emit transactionRemoved(t);
}

void KNSBackend::installApplication(AbstractResource* app)
{
    Transaction* t = new Transaction(app, InstallApp);
    emit transactionAdded(t);
    KNSResource* r = qobject_cast<KNSResource*>(app);
    Q_ASSERT(r->entry());
    m_manager->installEntry(*r->entry());
    emit transactionRemoved(t);
}

void KNSBackend::installApplication(AbstractResource* app, const QHash< QString, bool >& addons)
{
    installApplication(app);
}

AbstractResource* KNSBackend::resourceByPackageName(const QString& name) const
{
    return m_resourcesByName[name];
}

bool KNSBackend::providesResouce(AbstractResource* resource) const
{
    return qobject_cast<KNSResource*>(resource);
}

QList<Transaction*> KNSBackend::transactions() const
{
    return QList<Transaction*>();
}

QPair<TransactionStateTransition, Transaction*> KNSBackend::currentTransactionState() const
{
    return QPair<TransactionStateTransition, Transaction*>(FinishedCommitting, 0);
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

AbstractReviewsBackend* KNSBackend::reviewsBackend() const
{
    return m_reviews;
}

QStringList KNSBackend::searchPackageName(const QString& searchText)
{
    QStringList ret;
    foreach(AbstractResource* r, m_resourcesByName) {
        if(r->name().contains(searchText) || r->comment().contains(searchText))
            ret += r->packageName();
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
