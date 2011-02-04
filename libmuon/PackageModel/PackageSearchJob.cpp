/***************************************************************************
 *   Copyright © 2011 Jonathan Thomas <echidnaman@kubuntu.org>             *
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

#include "PackageSearchJob.h"

#include <QStringList>

#include <KDebug>

PackageSearchJob::PackageSearchJob(QObject *parent)
    : ThreadWeaver::Job(parent)
    , m_backend(0)
    , m_searchType(InvalidType)
{
}

PackageSearchJob::~PackageSearchJob()
{
}

void PackageSearchJob::run()
{
    switch (m_searchType) {
    case NameSearch:
        foreach (QApt::Package *package, m_searchPackages) {
            if (package->name().contains(m_searchText)) {
                m_searchResults.append(package);
            }
        }
        break;
    case NameDescSearch:
        break;
    case MaintainerSearch:
        foreach (QApt::Package *package, m_searchPackages) {
            if (package->maintainer().contains(m_searchText)) {
                m_searchResults.append(package);
            }
        }
        break;
    case VersionSearch:
        break;
    case DependsSearch:
        foreach (QApt::Package *package, m_searchPackages) {
            if (package->dependencyList(true).contains(m_searchText)) {
                m_searchResults.append(package);
            }
        }
        break;
    case ProvidesSearch:
        foreach (QApt::Package *package, m_searchPackages) {
            if (package->providesList().contains(m_searchText)) {
                m_searchResults.append(package);
            }
        }
        break;
    case QuickSearch:
        m_searchResults = m_backend->search(m_searchText);
    case InvalidType:
    default:
        break;
    }
}

void PackageSearchJob::setBackend(QApt::Backend *backend)
{
    m_backend = backend;
}

void PackageSearchJob::setSearchPackages(const QApt::PackageList &searchPackages)
{
    m_searchPackages = searchPackages;
}

void PackageSearchJob::setSearchText(const QString &searchText)
{
    m_searchText = searchText;
}

void PackageSearchJob::setSearchType(SearchType type)
{
    m_searchType = type;
}

QApt::PackageList PackageSearchJob::searchResults()
{
    return m_searchResults;
}
