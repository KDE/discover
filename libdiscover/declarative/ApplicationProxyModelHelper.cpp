/*
 *   Copyright (C) 2012 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library/Lesser General Public License
 *   version 2, or (at your option) any later version, as published by the
 *   Free Software Foundation
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details
 *
 *   You should have received a copy of the GNU Library/Lesser General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "ApplicationProxyModelHelper.h"
#include <resources/ResourcesModel.h>
#include <QDebug>

ApplicationProxyModelHelper::ApplicationProxyModelHelper(QObject* parent)
    : ResourcesProxyModel(parent)
{
    connect(this, &QAbstractItemModel::rowsInserted, this, &ApplicationProxyModelHelper::countChanged);
    connect(this, &QAbstractItemModel::rowsRemoved, this, &ApplicationProxyModelHelper::countChanged);
    setDynamicSortFilter(false);
}

QHash<int, QByteArray> ApplicationProxyModelHelper::roleNames() const
{
    return ResourcesModel::global()->roleNames();
}

void ApplicationProxyModelHelper::componentComplete()
{
    if(!m_sortRoleString.isEmpty())
        setStringSortRole_hack(m_sortRoleString);

    setSearch(lastSearch());
    setDynamicSortFilter(true);
    sortModel();
}

void ApplicationProxyModelHelper::sortModel()
{
    QSortFilterProxyModel::sort(0, sortOrder());
}

int ApplicationProxyModelHelper::stringToRole(const QByteArray& strRole) const
{
    return roleNames().key(strRole);
}

QByteArray ApplicationProxyModelHelper::roleToString(int role) const
{
    return roleNames().value(role);
}

void ApplicationProxyModelHelper::setSortRole_hack(int role)
{
    if (role != sortRole()) {
        setSortRole(role);
        emit sortRoleChanged();
    }
}

void ApplicationProxyModelHelper::setSortOrder_hack(Qt::SortOrder order)
{
    sort(0, order);
    emit sortOrderChanged();
}

void ApplicationProxyModelHelper::setStringSortRole_hack(const QString& role)
{
    setSortRole_hack(stringToRole(role.toUtf8()));
    m_sortRoleString = role;
}

QString ApplicationProxyModelHelper::stringSortRole() const
{
    return QString::fromLatin1(roleToString(sortRole()));
}
