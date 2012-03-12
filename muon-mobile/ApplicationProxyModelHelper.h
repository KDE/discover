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

#include <ApplicationModel/ApplicationProxyModel.h>


class ApplicationProxyModelHelper : public ApplicationProxyModel
{
    Q_OBJECT
    Q_PROPERTY(int stateFilter READ stateFilter WRITE setStateFilter_hack)
    Q_PROPERTY(int sortRole READ sortRole WRITE setSortRole_hack NOTIFY sortRoleChanged)
    public:
        void setStateFilter_hack(int state);
        explicit ApplicationProxyModelHelper(QObject* parent = 0);
        
        Q_SCRIPTABLE void sortModel(int column, int order);
        Q_SCRIPTABLE Application* applicationAt(int row);
        Q_SCRIPTABLE int stringToRole(const QByteArray& strRole) const;
        Q_SCRIPTABLE QByteArray roleToString(int role) const;
        
        void setSortRole_hack(int role);
    signals:
        void sortRoleChanged();
};

#endif // APPLICATIONPROXYMODELHELPER_H
