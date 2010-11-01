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

class PackageDetails
{
public:
    PackageDetails()
    : percentage(0), size(0), status(0) {};
    QString name;
    int percentage;
    QString URI;
    double size;
    int status;
};

DownloadModel::DownloadModel(QObject *parent)
: QAbstractListModel(parent)
{
}

QVariant DownloadModel::data(const QModelIndex& index, int role) const
{
    PackageDetails details = m_packageList.at(index.row());
    switch (role) {
    case NameRole:
        return QVariant(details.name);
    case PercentRole:
        return QVariant(details.percentage);
    case URIRole:
        return QVariant(details.URI);
    case SizeRole:
        return QVariant(details.size);
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
            return i18n("Package");
        case 1:
            return i18n("Location");
        case 2:
            return i18n("Size");
        case 3:
            return i18n("Progress");
        default:
            return QVariant();
    }
}

void DownloadModel::updatePackageDetails(const QString &package, int percentage,
                                         const QString &URI, double size, int status)
{
    bool newPackage = true;
    for (int i = 0; i < m_packageList.size(); ++i) {
        // URI should be unique
        if (m_packageList.at(i).URI != URI) {
            continue;
        }

        newPackage = false;
        m_packageList[i].name = package;
        m_packageList[i].percentage = percentage;
        m_packageList[i].size = size;
        m_packageList[i].status = status;
        // If we get more than 10 columns we'll have to bump this.
        // ... but that's really not likely...
        emit dataChanged(index(i, 0), index(i, 9));
        break;
    }

    if (newPackage) {
        beginInsertRows(QModelIndex(), m_packageList.count(), m_packageList.count());
        PackageDetails details;
        details.name = package;
        details.percentage = percentage;
        details.URI = URI;
        details.size = size;
        details.status = status;
        m_packageList.append(details);
        endInsertRows();
    }

    return;
}

void DownloadModel::clear()
{
    m_packageList.clear();
}

int DownloadModel::rowCount(const QModelIndex& /*parent*/) const
{
    return m_packageList.size();
}

int DownloadModel::columnCount(const QModelIndex& /*parent*/) const
{
    return 4;
}

#include "DownloadModel.moc"
