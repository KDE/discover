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

#ifndef PACKAGESEARCHJOB_H
#define PACKAGESEARCHJOB_H

#include <threadweaver/Job.h>

#include <LibQApt/Backend>

#include "../libmuonprivate_export.h"

class MUONPRIVATE_EXPORT PackageSearchJob : public ThreadWeaver::Job
{
Q_OBJECT
public:
    enum SearchType {
        InvalidType = 0,
        NameSearch = 1,
        NameDescSearch = 2,
        MaintainerSearch = 3,
        VersionSearch = 4,
        DependsSearch = 5,
        ProvidesSearch = 6,
        QuickSearch = 7
    };

    PackageSearchJob(QObject *parent = 0);
    ~PackageSearchJob();

    void setBackend(QApt::Backend *backend);
    void setSearchPackages(const QApt::PackageList &packages);
    void setSearchText(const QString &searchText);
    void setSearchType(SearchType type);

    QApt::PackageList searchResults();

protected:
    void run();

private:
    QApt::Backend *m_backend; // For quicksearch via xapian

    QApt::PackageList m_searchPackages;
    QString m_searchText;
    SearchType m_searchType;

    QApt::PackageList m_searchResults;
};

#endif
