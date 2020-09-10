/*
 *   SPDX-FileCopyrightText: 2014 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "ActionsModel.h"
#include "resources/ResourcesModel.h"
#include "utils.h"
#include <QAction>
#include "libdiscover_debug.h"

ActionsModel::ActionsModel(QObject* parent)
    : QAbstractListModel(parent)
    , m_priority(-1)
{
    connect(ResourcesModel::global(), &ResourcesModel::backendsChanged, this, &ActionsModel::reload);
}

QHash< int, QByteArray > ActionsModel::roleNames() const
{
    return { { Qt::UserRole, "action" }};
}

QVariant ActionsModel::data(const QModelIndex& index, int role) const
{
    if(!index.isValid() || role!=Qt::UserRole)
        return QVariant();
    return QVariant::fromValue<QObject*>(m_filteredActions[index.row()]);
}

int ActionsModel::rowCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : m_filteredActions.count();
}

void ActionsModel::setActions(const QVariant& actions)
{
    if (m_actions == actions) {
        return;
    }
    m_actions = actions;

    reload();
    Q_EMIT actionsChanged(m_actions);
}

void ActionsModel::reload()
{
    QList<QAction*> actions = m_actions.value<QList<QAction*>>();
    if (m_priority>=0) {
        actions = kFilter<QList<QAction*>>(actions, [this](QAction* action){ return action->priority() == m_priority; });
    }
    actions = kFilter<QList<QAction*>>(actions, [](QAction* action){ return action->isVisible(); });
    if (actions == m_filteredActions)
        return;


    beginResetModel();
    m_filteredActions = actions;
    endResetModel();

    for(auto a : qAsConst(m_filteredActions)) {
        connect(a, &QAction::changed, this, &ActionsModel::reload, Qt::UniqueConnection);
    }
}

int ActionsModel::filterPriority() const
{
    return m_priority;
}

void ActionsModel::setFilterPriority(int p)
{
    if (m_priority != p) {
        m_priority = p;
        reload();
        Q_EMIT filterPriorityChanged();
    }
}
