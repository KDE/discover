/*
 *   SPDX-FileCopyrightText: 2022 Suhaas Joshi <joshiesuhaas0@gmail.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "FlatpakPermission.h"

#include <KLocalizedString>

FlatpakPermission::FlatpakPermission(QString brief, QString description, QString icon)
    : m_brief(brief)
    , m_description(description)
    , m_icon(icon)
{
}

QString FlatpakPermission::icon() const
{
    return m_icon;
}

QString FlatpakPermission::brief() const
{
    return m_brief;
}

QString FlatpakPermission::description() const
{
    return m_description;
}

FlatpakPermissionsModel::FlatpakPermissionsModel(QVector<FlatpakPermission> permissions)
    : QAbstractListModel()
    , m_permissions(permissions)
{
}

int FlatpakPermissionsModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        return 0;
    }
    return m_permissions.count();
}

QVariant FlatpakPermissionsModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) {
        return QVariant();
    }

    switch (role) {
    case Roles::BriefRole:
        return m_permissions.at(index.row()).brief();
    case Roles::DescriptionRole:
        return m_permissions.at(index.row()).description();
    case Roles::IconRole:
        return m_permissions.at(index.row()).icon();
    }
    return QVariant();
}

QHash<int, QByteArray> FlatpakPermissionsModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[Roles::BriefRole] = "brief";
    roles[Roles::DescriptionRole] = "description";
    roles[Roles::IconRole] = "icon";
    return roles;
}
