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
#include <knewstuff3/downloadmanager.h>

KNSBackend::KNSBackend(const QString& configName, QObject* parent)
    : AbstractResourcesBackend(parent)
    , m_reviews(new KNSReviews(this))
{
    m_manager = new KNS3::DownloadManager(configName, this);
    connect(m_manager, SIGNAL(searchResult(KNS3::Entry::List)), SLOT(receivedEntries(KNS3::Entry::List)));
    m_manager->search();
    m_page = 0;
}

void KNSBackend::receivedEntries(const KNS3::Entry::List& entries)
{
    if(entries.isEmpty()) {
        emit backendReady();
        return;
    }
    
    foreach(const KNS3::Entry& entry, entries) {
        m_resourcesByName.insert(entry.id(), new KNSResource(entry, this));
    }
    ++m_page;
    m_manager->search(m_page);
}

void KNSBackend::cancelTransaction(AbstractResource* app)
{

}

void KNSBackend::removeApplication(AbstractResource* app)
{

}

void KNSBackend::installApplication(AbstractResource* app)
{

}

void KNSBackend::installApplication(AbstractResource* app, const QHash< QString, bool >& addons)
{

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
    return QStringList(m_resourcesByName.keys()).filter(searchText);
}

QVector< AbstractResource* > KNSBackend::allResources() const
{
    return m_resourcesByName.values().toVector();
}
