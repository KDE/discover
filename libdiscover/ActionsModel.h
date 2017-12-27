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

#ifndef ACTIONSMODEL_H
#define ACTIONSMODEL_H

#include <QAbstractListModel>
#include <QQmlParserStatus>
#include "discovercommon_export.h"

class QAction;

class DISCOVERCOMMON_EXPORT ActionsModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(QList<QAction*> actions READ actions WRITE setActions NOTIFY actionsChanged)
    Q_PROPERTY(int filterPriority READ filterPriority WRITE setFilterPriority)
    public:
        explicit ActionsModel(QObject* parent = nullptr);

        QHash<int, QByteArray> roleNames() const override;
        QVariant data(const QModelIndex& index, int role) const override;
        int rowCount(const QModelIndex& parent = QModelIndex()) const override;

        void setFilterPriority(int p);
        int filterPriority() const;

        void setActions(const QList<QAction*>& actions);
        QList<QAction*> actions() const { return m_actions; }

    Q_SIGNALS:
        void actionsChanged(const QList<QAction*>& actions);

    private:
        void reload();

        QList<QAction*> m_actions;
        QList<QAction*> m_filteredActions;
        int m_priority;
};

#endif
