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
import QtQuick.Controls 1.0
import QtQuick.Layouts 1.0
import org.kde.kquickcontrolsaddons 2.0
import org.kde.kirigami 1.0 as Kirigami
import "navigation.js" as Navigation

RowLayout {
    id: bread
    property alias model: rep.model

    readonly property Action homeAction: Kirigami.Action {
        text: i18n("Home")
        onTriggered: Navigation.openHome()
    }

    Repeater {
        id: rep

        delegate: RowLayout {
            spacing: 0
            Text {
                visible: index > 0
                text: ">"
                color: button.textColor
            }
            LinkButton {
                id: button
                Layout.fillHeight: true
                action: modelData
            }
        }
    }
}
