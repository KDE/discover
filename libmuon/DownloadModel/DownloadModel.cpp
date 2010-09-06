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
#include <KLocale>

DownloadModel::DownloadModel(QObject *parent)
: QAbstractListModel(parent)
{
}

QVariant DownloadModel::data(const QModelIndex& index, int role) const
{
    QHash<int, QVariant> details = m_packageList.at(index.row());
    switch (role) {
    case NameRole:
        return QVariant(details[NameRole]);
    case PercentRole:
        return QVariant(details[PercentRole]);
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
            return QVariant(i18n("Package"));
        case 1:
            return QVariant(i18n("Downloaded"));
        default:
            return QVariant();
    }
}

void DownloadModel::updatePercentage(const QString &package, int percentage, const QString &URI,
                                     double size, int status)
{
    bool newPackage = true;
    for (int i = 0; i < m_packageList.size(); ++i) {
        if (m_packageList[i].value(URIRole).toString() != URI) {
            continue;
        }

        newPackage = false;
        m_packageList[i].insert(NameRole, package);
        m_packageList[i].insert(PercentRole, percentage);
        m_packageList[i].insert(SizeRole, size);
        // TODO value[StatusRole] = status;
        emit dataChanged(index(i, 1), index(i, 1));
        break;
    }

    if (newPackage) {
        beginInsertRows(QModelIndex(), m_packageList.count(), m_packageList.count());
        QHash<int, QVariant> details;
        details[URIRole] = URI;
        details[NameRole] = package;
        details[PercentRole] = percentage;
        details[SizeRole] = size;
        // TODO details[StatusRole] = status;
        m_packageList.append(details);
        endInsertRows();
    }

    return;
}

int DownloadModel::rowCount(const QModelIndex& /*parent*/) const
{
    return m_packageList.size();
}

int DownloadModel::columnCount(const QModelIndex& /*parent*/) const
{
    return 2;
}

#include "DownloadModel.moc"
