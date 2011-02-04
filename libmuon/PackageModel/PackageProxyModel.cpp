/***************************************************************************
 *   Copyright © 2010 Jonathan Thomas <echidnaman@kubuntu.org>             *
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

#include "PackageProxyModel.h"

// KDE includes
#include <KLocale>
#include <Solid/Device>
#include <Solid/DeviceInterface>
#include <threadweaver/ThreadWeaver.h>
#include <KDebug>

// LibQApt includes
#include <LibQApt/Backend>

// Own includes
#include "PackageModel.h"

static const int status_sort_magic = (QApt::Package::Installed
//                                       | QApt::Package::Outdated
                                      | QApt::Package::New);
bool packageStatusLessThan(QApt::Package *p1, QApt::Package *p2)
{
    return (p1->state() & (status_sort_magic))  <
           (p2->state() & (status_sort_magic));
};

static const int requested_sort_magic = (QApt::Package::ToInstall
                                         | QApt::Package::ToRemove
                                         | QApt::Package::ToKeep);

bool packageRequestedLessThan(QApt::Package *p1, QApt::Package *p2)
{
    return (p1->state() & (requested_sort_magic))  <
           (p2->state() & (requested_sort_magic));
};

PackageProxyModel::PackageProxyModel(QObject *parent)
    : QSortFilterProxyModel(parent)
    , m_backend(0)
    , m_stateFilter((QApt::Package::State)0)
    , m_sortByRelevancy(false)
    , m_fullSearch(false)
{
    m_weaver = ThreadWeaver::Weaver::instance();
    connect(m_weaver, SIGNAL(jobDone(ThreadWeaver::Job *)),
            this, SLOT(searchDone(ThreadWeaver::Job *)));
}

PackageProxyModel::~PackageProxyModel()
{
}

void PackageProxyModel::setBackend(QApt::Backend *backend)
{
    m_backend = backend;
    m_packages = static_cast<PackageModel *>(sourceModel())->packages();
}

void PackageProxyModel::search(const QString &text, PackageSearchJob::SearchType type)
{
    m_packages.clear();
    m_fullSearchResults.clear();

    // One-character searches will either provide way to may results for full
    // search or cause quicksearch to hang for a few seconds.
    if (!(text.size() > 1)) {
        m_packages =  static_cast<PackageModel *>(sourceModel())->packages();
        m_sortByRelevancy = false;
        m_fullSearch = false;

        invalidate();
        return;
    }

    // We don't need multiple buckets for quicksearch. Xapian doesn't do the
    // search on the package level and can do most searches in a few ms in
    // one thread.
    if (type == PackageSearchJob::QuickSearch) {
        m_sortByRelevancy = true;
        m_expectedJobs = 1;

        PackageSearchJob *job = new PackageSearchJob(this);
        job->setBackend(m_backend);
        job->setSearchText(text);
        job->setSearchType(type);

        m_weaver->enqueue(job);
        return;
    }

    m_fullSearch = true;
    // Set up the optimal number of threads for the processor
    const int numProcs =
            qMax(Solid::Device::listFromType(Solid::DeviceInterface::Processor).count(), 1);
    int numThreads = (2 + ((numProcs - 1) * 2));

    if (numThreads > m_weaver->maximumNumberOfThreads()) {
        m_weaver->setMaximumNumberOfThreads(numThreads);
    } else {
        numThreads = m_weaver->maximumNumberOfThreads();
    }

    QApt::PackageList allPackages = m_backend->availablePackages();

    int bucketSize = allPackages.size() / numThreads;
    int remainder = allPackages.size() % numThreads;

    m_expectedJobs = numThreads;

    // kDebug() << "Package Count:" << allPackages.size();
    // kDebug() << "Bucket size:" << bucketSize;
    // kDebug() << "Remainder:" << remainder;

    int packagesIndex = 0;
    for (int i = 0; i < numThreads; ++i) {
        QApt::PackageList jobList = allPackages.mid(packagesIndex, bucketSize);
        packagesIndex += bucketSize;

        // Give the remainder to our last job
        if (remainder) {
            if (i == (numThreads - 1)) {
                jobList.append(allPackages.mid(packagesIndex, remainder));
            }
        }

        PackageSearchJob *job = new PackageSearchJob(this);
        job->setBackend(m_backend);
        job->setSearchPackages(jobList);
        job->setSearchText(text);
        job->setSearchType(type);

        m_weaver->enqueue(job);
    }
}

void PackageProxyModel::searchDone(ThreadWeaver::Job *job)
{
    kDebug() << "Search done";
    PackageSearchJob *searchJob = (PackageSearchJob *)job;
    kDebug() << searchJob->searchResults().size();
    m_fullSearchResults.append(searchJob->searchResults());

    delete searchJob;

    m_expectedJobs--;

    if (!m_expectedJobs) {
        kDebug() << "setting results";
        m_packages = m_fullSearchResults;
        invalidate();
    }
}

void PackageProxyModel::setGroupFilter(const QString &filterText)
{
    m_groupFilter = filterText;
    invalidate();
}

void PackageProxyModel::setStateFilter(QApt::Package::State state)
{
    m_stateFilter = state;
    invalidate();
}

void PackageProxyModel::setOriginFilter(const QString &origin)
{
    m_originFilter = origin;
    invalidate();
}

bool PackageProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    //Our "main"-method
    QApt::Package *package = static_cast<PackageModel *>(sourceModel())->packageAt(sourceModel()->index(sourceRow, 1, sourceParent));
    //We have a package as internal pointer
    if (!package) {
        return false;
    }

    if (!m_groupFilter.isEmpty()) {
        if (!package->section().contains(m_groupFilter)) {
            return false;
        }
    }

    if (!m_stateFilter == 0) {
        if ((bool)(package->state() & m_stateFilter) == false) {
            return false;
        }
    }

    if (!m_originFilter.isEmpty()) {
        if (!(package->origin() == m_originFilter)) {
            return false;
        }
    }

    if (m_sortByRelevancy || m_fullSearch) {
        return m_packages.contains(package);
    }
    return true;
}

QApt::Package *PackageProxyModel::packageAt(const QModelIndex &index) const
{
    // Since our representation is almost bound to change, we need to grab the parent model's index
    QModelIndex sourceIndex = mapToSource(index);
    QApt::Package *package = static_cast<PackageModel *>(sourceModel())->packageAt(sourceIndex);
    return package;
}

void PackageProxyModel::reset()
{
    beginRemoveRows(QModelIndex(), 0, m_packages.size());
    m_packages =  static_cast<PackageModel *>(sourceModel())->packages();
    endRemoveRows();
    invalidate();
}

bool PackageProxyModel::lessThan(const QModelIndex &left, const QModelIndex &right) const
{
    PackageModel *model = static_cast<PackageModel *>(sourceModel());
    QApt::Package *leftPackage = model->packageAt(left);
    QApt::Package *rightPackage = model->packageAt(right);

    switch (left.column()) {
      case 0:
          if (m_sortByRelevancy) {
              // This is expensive for very large datasets. It takes about 3 seconds with 30,000 packages
              // The order in m_packages is based on relevancy when returned by m_backend->search()
              // Use this order to determine less than
              if (m_packages.indexOf(leftPackage) < m_packages.indexOf(rightPackage)) {
                  return false;
              } else {
                  return true;
              }
          } else {
              QString leftString = left.data(PackageModel::NameRole).toString();
              QString rightString = right.data(PackageModel::NameRole).toString();

              return leftString > rightString;
          }
      case 1:
          return packageStatusLessThan(leftPackage, rightPackage);
      case 2:
          return packageRequestedLessThan(leftPackage, rightPackage);
    }

    return false;
}
