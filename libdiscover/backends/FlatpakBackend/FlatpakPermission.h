/*
 *   SPDX-FileCopyrightText: 2022 Suhaas Joshi <joshiesuhaas0@gmail.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include <QAbstractListModel>
#include <QString>
#include <QVariant>
#include <QVector>

class FlatpakPermission
{
public:
    FlatpakPermission(const QString &brief, const QString &description, const QString &icon);
    QString icon() const;
    QString brief() const;
    QString description() const;

    bool operator==(const FlatpakPermission &other) const
    {
        return m_icon == other.m_icon && m_description == other.m_description && m_brief == other.m_brief;
    }

private:
    QString m_brief;
    QString m_description;
    QString m_icon;
};

class FlatpakPermissionsModel : public QAbstractListModel
{
public:
    FlatpakPermissionsModel(const QVector<FlatpakPermission> &permissions);

    enum Roles {
        BriefRole = Qt::UserRole + 1,
        DescriptionRole,
        ListRole,
        IconRole,
    };

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

private:
    QVector<FlatpakPermission> m_permissions;
};
