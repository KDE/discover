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

#include "SourcesModel.h"
#include <QtGlobal>
#include <QDebug>
#include <QAction>
#include "resources/AbstractSourcesBackend.h"

Q_GLOBAL_STATIC(SourcesModel, s_sources)

SourcesModel::SourcesModel(QObject* parent)
    : QAbstractListModel(parent)
{}

SourcesModel::~SourcesModel() = default;

QHash<int, QByteArray> SourcesModel::roleNames() const
{
    QHash<int, QByteArray> roles = QAbstractItemModel::roleNames();
    roles[SourceBackend] = "sourceBackend";
    return roles;
}

SourcesModel* SourcesModel::global()
{
    return s_sources;
}

void SourcesModel::addSourcesBackend(AbstractSourcesBackend* sources)
{
    if (m_sources.contains(sources))
        return;

    beginInsertRows(QModelIndex(), m_sources.size(), m_sources.size());
    m_sources += sources;
    endInsertRows();
    emit sourcesChanged();
}

QVariant SourcesModel::data(const QModelIndex& index, int role) const
{
    if(!index.isValid() || index.row()>=m_sources.count()) {
        return QVariant();
    }
    switch(role) {
        case Qt::DisplayRole:
            return m_sources[index.row()]->name();
        case SourceBackend:
            return QVariant::fromValue<QObject*>(m_sources[index.row()]);
    }

    return QVariant();
}

int SourcesModel::rowCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : m_sources.count();
}

QVariant SourcesModel::get(int row, const QByteArray& roleName)
{
    return data(index(row), roleNames().key(roleName));
}

QList<QObject*> SourcesModel::actions() const
{
    QList<QObject*> ret;
    for(AbstractSourcesBackend* b: m_sources) {
        foreach(QAction* action, b->actions())
            ret.append(action);
    }
    return ret;
}
