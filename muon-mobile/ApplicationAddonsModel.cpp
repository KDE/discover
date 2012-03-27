/***************************************************************************
 *   Copyright © 2012 Aleix Pol Gonzalez <aleixpol@blue-systems.com>       *
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

#include "ApplicationAddonsModel.h"
#include <Application.h>
#include <QDebug>

ApplicationAddonsModel::ApplicationAddonsModel(QObject* parent)
    : QAbstractListModel(parent)
{}

void ApplicationAddonsModel::setApplication(Application* app)
{
    m_state.clear();
    m_app = app;
    m_addons = m_app->addons();
    
    foreach(QApt::Package* p, m_addons) {
        m_state.insert(p, p->isInstalled());
    }
    
    QHash<int, QByteArray> roles = roleNames();
    roles.insert(Qt::CheckStateRole, "checked");
    setRoleNames(roles);
}

Application* ApplicationAddonsModel::application() const
{
    return m_app;
}

int ApplicationAddonsModel::rowCount(const QModelIndex& parent) const
{
    return parent.isValid()? 0 : m_addons.size();
}

QVariant ApplicationAddonsModel::data(const QModelIndex& index, int role) const
{
    if(!index.isValid() || index.row()>=m_addons.size())
        return QVariant();
    
    switch(role) {
        case Qt::DisplayRole:
            return m_addons[index.row()]->name();
        case Qt::ToolTipRole:
            return m_addons[index.row()]->shortDescription();
        case Qt::CheckStateRole:
            return m_addons[index.row()]->isInstalled();
    }
    
    return QVariant();
}

