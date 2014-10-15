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
#include <KLocalizedString>

// QApt includes
#include <QApt/Backend>

// Own includes
#include "PackageModel.h"
#include "MuonSettings.h"

constexpr int status_sort_magic = (QApt::Package::Installed |
                                   QApt::Package::New);

bool packageStatusLessThan(QApt::Package *p1, QApt::Package *p2)
{
    return (p1->state() & (status_sort_magic))  <
           (p2->state() & (status_sort_magic));
}

constexpr int requested_sort_magic = (QApt::Package::ToInstall
                                         | QApt::Package::ToRemove
                                         | QApt::Package::ToKeep);

bool packageRequestedLessThan(QApt::Package *p1, QApt::Package *p2)
{
    return (p1->state() & (requested_sort_magic))  <
           (p2->state() & (requested_sort_magic));
}

PackageProxyModel::PackageProxyModel(QObject *parent)
    : QSortFilterProxyModel(parent)
    , m_backend(0)
    , m_stateFilter((QApt::Package::State)0)
    , m_sortByRelevancy(false)
    , m_useSearchResults(false)
{
}

void PackageProxyModel::setBackend(QApt::Backend *backend)
{
    m_backend = backend;
    m_packages = static_cast<PackageModel *>(sourceModel())->packages();
}

void PackageProxyModel::search(const QString &searchText)
{
    // 1-character searches are painfully slow. >= 2 chars are fine, though
    if (searchText.size() > 1) {
        m_searchPackages = m_backend->search(searchText);
        m_sortByRelevancy = true;
        m_useSearchResults = true;
    } else {
        m_searchPackages.clear();
        m_packages =  static_cast<PackageModel *>(sourceModel())->packages();
        m_sortByRelevancy = false;
        m_useSearchResults = false;
    }

    invalidate();
}

void PackageProxyModel::setSortByRelevancy(bool enabled)
{
    m_sortByRelevancy = enabled;
    invalidate();
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

void PackageProxyModel::setArchFilter(const QString &arch)
{
    m_archFilter = arch;
    invalidate();
}

bool PackageProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    // Our "main"-method
    QApt::Package *package = static_cast<PackageModel *>(sourceModel())->packageAt(sourceModel()->index(sourceRow, 1, sourceParent));
    // We have a package as internal pointer
    if (!package) {
        return false;
    }

    if (!m_groupFilter.isEmpty()) {
        if (!QString(package->section()).contains(m_groupFilter)) {
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

    if (!m_archFilter.isEmpty()) {
        if (!(package->architecture() == m_archFilter)) {
            return false;
        }
    }

    if (!MuonSettings::self()->showMultiArchDupes()) {
        if (package->isMultiArchDuplicate())
            return false;
    }

    if (m_useSearchResults)
        return m_searchPackages.contains(package);

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
              return (m_searchPackages.indexOf(leftPackage) > m_searchPackages.indexOf(rightPackage));
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
