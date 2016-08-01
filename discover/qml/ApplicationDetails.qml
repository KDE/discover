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

import QtQuick 2.1
import QtQuick.Controls 1.1
import QtQuick.Layouts 1.1
import QtQuick.Window 2.1
import org.kde.discover.app 1.0 as Discover
import org.kde.discover 1.0

ColumnLayout
{
    property QtObject application
    spacing: 1

    Label {
        text: i18n("Size: %1", application.sizeDescription)
    }
    Label {
        visible: text.length>0
        text: application.license ? i18n("License: %1", application.license) : ""
    }
    Label {
        text: i18n("Version: %1", application.isInstalled ? application.installedVersion : application.availableVersion)
    }
}
