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

#include "PackageModel.h"

#include <KIcon>
#include <KLocale>
#include <KDebug>

PackageModel::PackageModel(QObject *parent)
        : QAbstractListModel(parent)
        , m_packages()
{
}

PackageModel::~PackageModel()
{
}

int PackageModel::rowCount(const QModelIndex & /*parent*/) const
{
    return m_packages.count();
}

int PackageModel::columnCount(const QModelIndex & /*parent*/) const
{
    return 3;
}

QVariant PackageModel::data(const QModelIndex & index, int role) const
{
    switch (role) {
        case NameRole:
            return m_packages[index.row()]->name();
        case IconRole:
            return KIcon("application-x-deb");
        case DescriptionRole:
            return m_packages[index.row()]->shortDescription();
        case StatusRole:
        case ActionRole:
            return m_packages[index.row()]->state();
        case Qt::ToolTipRole:
            return QVariant();
        }

    return QVariant();
}

QVariant PackageModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    Q_UNUSED(orientation);
    if (role == Qt::DisplayRole) {
        switch(section) {
            case 0:
                return QVariant(i18n("Package"));
            case 1:
                return QVariant(i18n("Status"));
            case 2:
                return QVariant(i18n("Requested"));
        }
    }
    return QVariant();
}

void PackageModel::addPackage(QApt::Package *package)
{
    beginInsertRows(QModelIndex(), m_packages.count(), m_packages.count());
    m_packages.append(package);
    endInsertRows();
}

void PackageModel::removePackage(QApt::Package *package)
{
    int index = m_packages.indexOf(package);
    if (index > -1) {
        beginRemoveRows(QModelIndex(), index, index);
        m_packages.removeAt(index);
        endRemoveRows();
    }
}

QApt::Package *PackageModel::packageAt(const QModelIndex &index)
{
    return m_packages[index.row()];
}

#include "PackageModel.moc"
