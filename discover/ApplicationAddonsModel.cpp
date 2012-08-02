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
#include "BackendsSingleton.h"
#include <resources/ResourcesModel.h>
#include <resources/PackageState.h>
#include <resources/AbstractResource.h>
#include <QDebug>

ApplicationAddonsModel::ApplicationAddonsModel(QObject* parent)
    : QAbstractListModel(parent)
    , m_app(0)
{}

void ApplicationAddonsModel::setApplication(AbstractResource* app)
{
    discardChanges();
    m_app = app;
    m_initial = m_app->addonsInformation();
    
    QHash<int, QByteArray> roles = roleNames();
    roles.insert(Qt::CheckStateRole, "checked");
    setRoleNames(roles);
    emit applicationChanged();
}

AbstractResource* ApplicationAddonsModel::application() const
{
    return m_app;
}

int ApplicationAddonsModel::rowCount(const QModelIndex& parent) const
{
    return parent.isValid()? 0 : m_initial.size();
}

QVariant ApplicationAddonsModel::data(const QModelIndex& index, int role) const
{
    if(!index.isValid() || index.row()>=m_initial.size())
        return QVariant();
    
    switch(role) {
        case Qt::DisplayRole:
            return m_initial[index.row()].name();
        case Qt::ToolTipRole:
            return m_initial[index.row()].description();
        case Qt::CheckStateRole: {
            QHash<QString, bool>::const_iterator it = m_state.constFind(m_initial[index.row()].name());
            if(it==m_state.constEnd()) {
                return m_initial[index.row()].isInstalled();
            } else {
                return it.value();
            }
        }
    }
    
    return QVariant();
}

void ApplicationAddonsModel::discardChanges()
{
    //dataChanged should suffice, but it doesn't
    beginResetModel();
    m_state.clear();
    emit stateChanged();
    endResetModel();
}

void ApplicationAddonsModel::applyChanges()
{
    BackendsSingleton::self()->appsModel()->installApplication(m_app, m_state);
}

void ApplicationAddonsModel::changeState(const QString& packageName, bool installed)
{
    QList<PackageState>::const_iterator it=m_initial.constBegin(), itEnd=m_initial.constEnd();
    for(; it!=itEnd; ++it) {
        if(it->name()==packageName)
            break;
    }
    
    bool restored = it->isInstalled()==installed;
    if(restored)
        m_state.remove(packageName);
    else
        m_state.insert(packageName, installed);
    emit stateChanged();
}

bool ApplicationAddonsModel::hasChanges() const
{
    return !m_state.isEmpty();
}

bool ApplicationAddonsModel::isEmpty() const
{
    return m_initial.isEmpty();
}
