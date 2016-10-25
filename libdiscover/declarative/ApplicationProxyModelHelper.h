/*
 *   Copyright (C) 2012 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library/Lesser General Public License
 *   version 2, or (at your option) any later version, as published by the
 *   Free Software Foundation
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details
 *
 *   You should have received a copy of the GNU Library/Lesser General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef APPLICATIONPROXYMODELHELPER_H
#define APPLICATIONPROXYMODELHELPER_H

#include <resources/ResourcesProxyModel.h>
#include <QQmlParserStatus>

class ApplicationProxyModelHelper : public ResourcesProxyModel, public QQmlParserStatus
{
    Q_OBJECT
    Q_INTERFACES(QQmlParserStatus)
    Q_PROPERTY(QString stringSortRole READ stringSortRole WRITE setStringSortRole_hack NOTIFY sortRoleStringChanged)
    public:
        void setStateFilter_hack(int state);
        explicit ApplicationProxyModelHelper(QObject* parent = nullptr);
        
        void setStringSortRole_hack(const QString& role);
        QString stringSortRole() const;

        void classBegin() override {}
        void componentComplete() override;

    Q_SIGNALS:
        void sortRoleStringChanged();

    private:
        int stringToRole(const QByteArray& strRole) const;
        QByteArray roleToString(int role) const;
        
        QString m_sortRoleString;
};

#endif // APPLICATIONPROXYMODELHELPER_H
