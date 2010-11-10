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

#include "ApplicationProxyModel.h"

#include <KDebug>

// Own includes
#include "ApplicationModel.h"

ApplicationProxyModel::ApplicationProxyModel(QObject *parent)
    : QSortFilterProxyModel(parent)
    , m_backend(0)
    , m_stateFilter((QApt::Package::State)0)
{
}

ApplicationProxyModel::~ApplicationProxyModel()
{
}

void ApplicationProxyModel::setBackend(QApt::Backend *backend)
{
    m_backend = backend;
    m_apps = static_cast<ApplicationModel *>(sourceModel())->applications();
}

void ApplicationProxyModel::setStateFilter(QApt::Package::State state)
{
    m_stateFilter = state;
    invalidate();
}

void ApplicationProxyModel::setOriginFilter(const QString &origin)
{
    m_originFilter = origin;
    invalidate();
}

bool ApplicationProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    Application *application = static_cast<ApplicationModel *>(sourceModel())->applicationAt(sourceModel()->index(sourceRow, 0, sourceParent));
    //We have a package as internal pointer
    if (!application || !application->package()) {
        kDebug() << "no application";
        return false;
    }

    if (m_stateFilter) {
        if ((bool)(application->package()->state() & m_stateFilter) == false) {
            return false;
        }
    }

    if (!m_originFilter.isEmpty()) {
        if (application->package()->origin() != m_originFilter) {
            return false;
        }
    }

    return true;
}

Application *ApplicationProxyModel::applicationAt(const QModelIndex &index) const
{
    // Since our representation is almost bound to change, we need to grab the parent model's index
    QModelIndex sourceIndex = mapToSource(index);
    Application *application = static_cast<ApplicationModel *>(sourceModel())->applicationAt(sourceIndex);
    return application;
}

void ApplicationProxyModel::reset()
{
    beginRemoveRows(QModelIndex(), 0, m_apps.size());
    m_apps = static_cast<ApplicationModel *>(sourceModel())->applications();
    endRemoveRows();
    invalidate();
}

void ApplicationProxyModel::parentDataChanged()
{
    m_apps = static_cast<ApplicationModel *>(sourceModel())->applications();
    invalidate();
}

bool ApplicationProxyModel::lessThan(const QModelIndex &left, const QModelIndex &right) const
{
    QString leftString = left.data(ApplicationModel::NameRole).toString();
    QString rightString = right.data(ApplicationModel::NameRole).toString();

    return (QString::localeAwareCompare(leftString, rightString) < 0);
}
