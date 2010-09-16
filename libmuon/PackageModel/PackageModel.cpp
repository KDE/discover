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

PackageModel::PackageModel(QObject *parent)
    : QAbstractListModel(parent)
    , m_packages(QApt::PackageList())
{
}

PackageModel::~PackageModel()
{
}

int PackageModel::rowCount(const QModelIndex & /*parent*/) const
{
    return m_packages.size();
}

int PackageModel::columnCount(const QModelIndex & /*parent*/) const
{
    return 3;
}

QVariant PackageModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) {
        return false;
    }
    switch (role) {
    case NameRole:
        return m_packages.at(index.row())->latin1Name();
    case IconRole:
        return KIcon("application-x-deb");
    case DescriptionRole:
        return m_packages.at(index.row())->shortDescription();
    case StatusRole:
    case ActionRole:
        return m_packages.at(index.row())->state();
    case Qt::ToolTipRole:
        return QVariant();
    }

    return QVariant();
}

QVariant PackageModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    Q_UNUSED(orientation);
    if (role == Qt::DisplayRole) {
        switch (section) {
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

void PackageModel::setPackages(const QApt::PackageList &list)
{
    beginResetModel();
    m_packages = list;
    endResetModel();
}

void PackageModel::clear()
{
    beginRemoveRows(QModelIndex(), 0, m_packages.size() - 1);
    m_packages.clear();
    endRemoveRows();
}

QApt::Package *PackageModel::packageAt(const QModelIndex &index) const
{
    return m_packages.at(index.row());
}

QApt::PackageList PackageModel::packages() const
{
    return m_packages;
}

#include "PackageModel.moc"
