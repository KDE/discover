/*
 *   Copyright (C) 2012 Aleix Pol Gonzalez <aleixpol@kde.org>
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
#include <ApplicationModel/ApplicationModel.h>
#include "BackendsSingleton.h"
#include <QDebug>

ApplicationProxyModelHelper::ApplicationProxyModelHelper(QObject* parent)
    : ApplicationProxyModel(parent)
{
    setSourceModel(BackendsSingleton::self()->appsModel());
    setBackend(BackendsSingleton::self()->backend());
}

void ApplicationProxyModelHelper::sortModel(int column, int order)
{
    QSortFilterProxyModel::sort(column, (Qt::SortOrder) order);
}

void ApplicationProxyModelHelper::setStateFilter_hack(int state)
{
    setStateFilter((QApt::Package::State) state);
}
