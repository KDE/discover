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
#include <KDebug>

// LibQApt includes
#include <libqapt/backend.h>

// Own includes
#include "PackageModel.h"

PackageProxyModel::PackageProxyModel(QObject *parent, QApt::Backend *backend)
        : QSortFilterProxyModel(parent)
        , m_backend(backend)
        , m_packages(backend->availablePackages())
        , m_searchText(QString())
{
}

PackageProxyModel::~PackageProxyModel()
{
}

void PackageProxyModel::search(const QString &searchText)
{
    m_packages = m_backend->search(searchText);
    // Add packages manually via packageAt?
    invalidateFilter();
}

bool PackageProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    //Our "main"-method
    QApt::Package *package = static_cast<PackageModel*>(sourceModel())->packageAt(sourceModel()->index(sourceRow, 1, sourceParent));
    //We have a package as internal pointer
    if (!package) {
        return false;
    }

    bool result = (m_packages.isEmpty() || m_packages.contains(package));
    return result;
}

QApt::Package *PackageProxyModel::packageAt(const QModelIndex &index)
{
    QModelIndex sourceIndex = mapToSource(index);
    QApt::Package *package = static_cast<PackageModel*>(sourceModel())->packageAt(sourceIndex);
    return package;
}