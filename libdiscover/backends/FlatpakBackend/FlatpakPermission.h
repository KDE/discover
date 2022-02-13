/*
 *   SPDX-FileCopyrightText: 2022 Suhaas Joshi <joshiesuhaas0@gmail.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#ifndef FLATPAKPERMISSION_H
#define FLATPAKPERMISSION_H

#include <QAbstractListModel>
#include <QString>
#include <QVariant>
#include <QVector>

class FlatpakPermission
{
public:
    FlatpakPermission(QString name, QString category, QString value, QString brief, QString description, QString icon, QStringList list = QStringList());
    QString name() const;
    QString value() const;
    QString category() const;
    QString icon() const;
    QString brief() const;
    QString description() const;
    QStringList list() const;

private:
    QString m_name;
    QString m_category;
    QString m_value; // on/off, talk/own, environment variables etc
    QString m_brief;
    QString m_description;
    QString m_icon;
    QStringList m_list; // to store all buses/directories
};

class FlatpakPermissionsModel : public QAbstractListModel
{
public:
    FlatpakPermissionsModel(QVector<FlatpakPermission> permissions);

    enum Roles {
        BriefRole = Qt::UserRole + 1,
        DescriptionRole,
        ListRole,
        NameRole,
        ValueRole,
        EmptyListRole,
        IconRole,
        CategoryRole
    };

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    virtual QHash<int, QByteArray> roleNames() const override;

private:
    QVector<FlatpakPermission> m_permissions;
};

#endif // FLATPAKPERMISSION_H
