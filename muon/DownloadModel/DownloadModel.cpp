/***************************************************************************
 *   Copyright © 2010 Guillaume Martres <smarter@ubuntu.com>               *
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

#include "DownloadModel.h"

#include <KDebug>
#include <KLocalizedString>

DownloadModel::DownloadModel(QObject *parent)
: QAbstractListModel(parent)
{
}

QVariant DownloadModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() > m_itemList.size() || index.row() < 0) {
        return QVariant();
    }

    QApt::DownloadProgress details = m_itemList.at(index.row());
    switch (role) {
    case NameRole:
        return QVariant(details.shortDescription());
    case PercentRole:
        return QVariant(details.progress());
    case URIRole:
        return QVariant(details.uri());
    case SizeRole:
        return QVariant(details.fileSize());
    default:
        return QVariant();
    }
}

QVariant DownloadModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    Q_UNUSED(orientation);
    if (role != Qt::DisplayRole) {
        return QVariant();
    }

    switch (section) {
        case 0:
            return i18nc("@title:column", "Package");
        case 1:
            return i18nc("@title:column", "Location");
        case 2:
            return i18nc("@title:column", "Size");
        case 3:
            return i18nc("@title:column", "Progress");
        default:
            return QVariant();
    }
}

void DownloadModel::updateDetails(const QApt::DownloadProgress &details)
{
    bool newPackage = true;
    for (int i = 0; i < m_itemList.size(); ++i) {
        // URI should be unique
        if (m_itemList.at(i).uri() != details.uri()) {
            continue;
        }

        newPackage = false;
        m_itemList[i] = details;
        // If we get more than 10 columns we'll have to bump this.
        // ... but that's really not likely...
        emit dataChanged(index(i, 0), index(i, 9));
        break;
    }

    if (newPackage) {
        beginInsertRows(QModelIndex(), m_itemList.count(), m_itemList.count());
        m_itemList.append(details);
        endInsertRows();
    }

    return;
}

void DownloadModel::clear()
{
    m_itemList.clear();
}

int DownloadModel::rowCount(const QModelIndex& /*parent*/) const
{
    return m_itemList.size();
}

int DownloadModel::columnCount(const QModelIndex& /*parent*/) const
{
    return 4;
}
