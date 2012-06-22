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

#include "OCSBackend.h"
#include "OCSResource.h"
#include <QStringList>

#include <attica/providermanager.h>
#include <attica/content.h>
#include <KDebug>

OCSBackend::OCSBackend(QObject* parent)
    : AbstractResourcesBackend(parent)
    , m_currentPage(0)
{
    m_manager = new Attica::ProviderManager;
    m_manager->addProviderFile(QUrl("http://collect.kde.org/org.plasma.share/providers.xml"));
    m_manager->addProviderFile(QUrl("http://collect.kde.org/kdevelop-qthelp/providers.xml"));
    m_manager->addProviderFile(QUrl("http://synchrotron/jacknjoe-webapps/providers.xml"));

    connect(m_manager, SIGNAL(defaultProvidersLoaded()), SLOT(providersLoaded()));
    m_manager->loadDefaultProviders();
}

void OCSBackend::providersLoaded()
{
    if (!m_manager->providers().isEmpty()) {
        m_provider = m_manager->providerByUrl(QUrl("https://api.opendesktop.org/v1/"));
//         m_provider = m_manager->providerByUrl(QUrl("http://synchrotron/org.plasma.share/v1/"));
//         m_provider = m_manager->providerByUrl(QUrl("http://synchrotron/jacknjoe-webapps/v1/"));
        if (!m_provider.isValid()) {
            kDebug() << "Could not find opendesktop.org provider.";
            return;
        }
        
        Attica::ListJob< Attica::Category >* job = m_provider.requestCategories();
        connect(job, SIGNAL(finished(Attica::BaseJob*)), SLOT(categoriesLoaded(Attica::BaseJob*)));
        job->start();
    }
}

void OCSBackend::categoriesLoaded(Attica::BaseJob* job)
{
    Attica::ListJob<Attica::Category>* listJob = static_cast<Attica::ListJob<Attica::Category>*>(job);
    m_categories = listJob->itemList();
    
    Attica::ListJob<Attica::Content>* jobContents = m_provider.searchContents(m_categories, QString(),
                                                                              Attica::Provider::Alphabetical, 0, 100);
    m_currentPage = 0;
    connect(jobContents, SIGNAL(finished(Attica::BaseJob*)), SLOT(loadContents(Attica::BaseJob*)));
    jobContents->start();
}

void OCSBackend::loadContents(Attica::BaseJob* job)
{
    Attica::ListJob<Attica::Content>* listJob = static_cast<Attica::ListJob<Attica::Content>*>(job);
    Attica::Content::List items = listJob->itemList();
    
    foreach(const Attica::Content& content, items) {
        m_resources += new OCSResource(content, this);
    }
    
    if(listJob->metadata().totalItems()>m_resources.count() && !items.isEmpty()) {
        Attica::ListJob<Attica::Content>* jobContents = m_provider.searchContents(m_categories, QString(),
                                                                              Attica::Provider::Alphabetical, 0, 100);
        connect(jobContents, SIGNAL(finished(Attica::BaseJob*)), SLOT(loadContents(Attica::BaseJob*)));
        jobContents->start();
    } else {
        qDebug() << "done!!";
        emit backendReady();
    }
}

QVector<AbstractResource*> OCSBackend::allResources() const
{
    QVector<AbstractResource*> ret;
    foreach(AbstractResource* ocsresource, m_resources)
        ret += ocsresource;
    return ret;
}

QStringList OCSBackend::searchPackageName(const QString& searchText)
{
    return QStringList();
}

bool OCSBackend::providesResouce(AbstractResource* resource) const
{
    return qobject_cast<OCSResource*>(resource);
}
