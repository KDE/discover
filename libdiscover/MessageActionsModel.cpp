/***************************************************************************
 *   Copyright © 2014 Aleix Pol Gonzalez <aleixpol@blue-systems.com>       *
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

#include "MessageActionsModel.h"
#include "resources/ResourcesModel.h"
#include <QAction>

MessageActionsModel::MessageActionsModel(QObject* parent)
    : QAbstractListModel(parent)
    , m_actions(ResourcesModel::global()->messageActions())
    , m_priority(-1)
{
    connect(ResourcesModel::global(), &ResourcesModel::backendsChanged, this, &MessageActionsModel::reload);
}

QHash< int, QByteArray > MessageActionsModel::roleNames() const
{
    return { { Qt::UserRole, "action" }};
}

QVariant MessageActionsModel::data(const QModelIndex& index, int role) const
{
    if(!index.isValid() || role!=Qt::UserRole)
        return QVariant();
    return QVariant::fromValue<QObject*>(m_actions[index.row()]);
}

int MessageActionsModel::rowCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : m_actions.count();
}


#include <QDebug>

void MessageActionsModel::reload()
{
    const auto actions = ResourcesModel::global()->messageActions();
    if (actions == m_actions)
        return;

    beginResetModel();
    m_actions = actions;
    if (m_priority>=0) {
        for(auto it=m_actions.begin(); it!=m_actions.end(); ) {
            if ((*it)->priority() == m_priority) {
                ++it;
            } else
                it = m_actions.erase(it);
        }
    }
    endResetModel();
}

int MessageActionsModel::filterPriority() const
{
    return m_priority;
}

void MessageActionsModel::setFilterPriority(int p)
{
    if (m_priority != p) {
        m_priority = p;
        reload();
    }
}
