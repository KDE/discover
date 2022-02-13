/*
 *   SPDX-FileCopyrightText: 2022 Suhaas Joshi <joshiesuhaas0@gmail.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "FlatpakPermission.h"

#include <KLocalizedString>

FlatpakPermission::FlatpakPermission(QString name, QString category, QString value, QString brief, QString description, QString icon, QStringList list)
    : m_name(name)
    , m_category(category)
    , m_value(value)
    , m_brief(brief)
    , m_description(description)
    , m_icon(icon)
    , m_list(list)
{
}

QString FlatpakPermission::name() const
{
    return m_name;
}

QString FlatpakPermission::category() const
{
    if (m_category == "filesystems") {
        return i18n("Directories");
    } else if (m_category == "System Bus Policy") {
        return i18n("System Buses");
    } else if (m_category == "Session Bus Policy") {
        return i18n("Session Buses");
    }
    return m_category;
}

QString FlatpakPermission::value() const
{
    return m_value;
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

QStringList FlatpakPermission::list() const
{
    return m_list;
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
    case Roles::ListRole:
        return m_permissions.at(index.row()).list().join("\n- ").prepend("- ");
    case Roles::NameRole:
        return m_permissions.at(index.row()).name();
    case Roles::ValueRole:
        return m_permissions.at(index.row()).value();
    case Roles::EmptyListRole:
        return !m_permissions.at(index.row()).list().isEmpty();
    case Roles::IconRole:
        return m_permissions.at(index.row()).icon();
    case Roles::CategoryRole:
        return m_permissions.at(index.row()).category();
    }
    return QVariant();
}

QHash<int, QByteArray> FlatpakPermissionsModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[Roles::BriefRole] = "brief";
    roles[Roles::DescriptionRole] = "description";
    roles[Roles::ListRole] = "list";
    roles[Roles::NameRole] = "name";
    roles[Roles::ValueRole] = "value";
    roles[Roles::EmptyListRole] = "emptylist";
    roles[Roles::IconRole] = "icon";
    roles[Roles::CategoryRole] = "category";
    return roles;
}
