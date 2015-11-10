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
#include <resources/ResourcesModel.h>
#include <resources/PackageState.h>
#include <resources/AbstractResource.h>
#include <Transaction/TransactionModel.h>
#include <QDebug>

ApplicationAddonsModel::ApplicationAddonsModel(QObject* parent)
    : QAbstractListModel(parent)
    , m_app(nullptr)
{
//     new ModelTest(this, this);

    connect(TransactionModel::global(), &TransactionModel::transactionRemoved, this, &ApplicationAddonsModel::transactionOver);
}

QHash< int, QByteArray > ApplicationAddonsModel::roleNames() const
{
    QHash<int, QByteArray> roles = QAbstractItemModel::roleNames();
    roles.insert(Qt::CheckStateRole, "checked");
    return roles;
}

void ApplicationAddonsModel::setApplication(AbstractResource* app)
{
    if (app == m_app)
        return;

    if (m_app)
        disconnect(m_app, nullptr, this, nullptr);

    m_app = app;
    resetState();
    emit applicationChanged();
}

void ApplicationAddonsModel::resetState()
{
    Q_ASSERT(m_app);
    beginResetModel();
    m_state.clear();
    m_initial = m_app->addonsInformation();
    endResetModel();

    emit stateChanged();
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
            PackageState init = m_initial[index.row()];
            AddonList::State state = m_state.addonState(init.name());
            if(state == AddonList::None) {
                return init.isInstalled();
            } else {
                return state == AddonList::ToInstall ? Qt::Checked : Qt::Unchecked;
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
    ResourcesModel::global()->installApplication(m_app, m_state);
}

void ApplicationAddonsModel::changeState(const QString& packageName, bool installed)
{
    auto it = m_initial.constBegin();
    for(; it != m_initial.constEnd(); ++it) {
        if(it->name()==packageName)
            break;
    }
    
    bool restored = it->isInstalled()==installed;

    if(restored)
        m_state.resetAddon(packageName);
    else
        m_state.addAddon(packageName, installed);

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

void ApplicationAddonsModel::transactionOver(Transaction* t)
{
    if (t->resource() != m_app)
        return;

    resetState();
}
