/*
 *   SPDX-FileCopyrightText: 2014 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#ifndef ACTIONSMODEL_H
#define ACTIONSMODEL_H

#include <QAbstractListModel>
#include <QQmlParserStatus>
#include "discovercommon_export.h"

class QAction;

class DISCOVERCOMMON_EXPORT ActionsModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(QVariant actions READ actions WRITE setActions NOTIFY actionsChanged)
    Q_PROPERTY(int filterPriority READ filterPriority WRITE setFilterPriority NOTIFY filterPriorityChanged)
    public:
        explicit ActionsModel(QObject* parent = nullptr);

        QHash<int, QByteArray> roleNames() const override;
        QVariant data(const QModelIndex& index, int role) const override;
        int rowCount(const QModelIndex& parent = QModelIndex()) const override;

        void setFilterPriority(int p);
        int filterPriority() const;

        void setActions(const QVariant& actions);
        QVariant actions() const { return m_actions; }

    Q_SIGNALS:
        void actionsChanged(const QVariant& actions);
        void filterPriorityChanged();

    private:
        void reload();

        QVariant m_actions;
        QList<QAction*> m_filteredActions;
        int m_priority;
};

#endif
