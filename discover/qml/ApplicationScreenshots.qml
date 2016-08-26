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
import QtQuick.Layouts 1.1
import QtQuick.Controls 1.1
import org.kde.discover 1.0
import org.kde.kirigami 1.0 as Kirigami

GridLayout {
    id: root
    property alias resource: screenshotsModel.application

    columnSpacing: Kirigami.Units.smallSpacing
    rowSpacing: Kirigami.Units.smallSpacing

    readonly property real side: Kirigami.Units.gridUnit*3
    property QtObject page
    visible: screenshotsModel.count>1

    readonly property var fu: Kirigami.OverlaySheet {
        id: overlay
        parent: root.page
        Image {
            id: overlayImage
            fillMode: Image.PreserveAspectFit
        }
    }

    Repeater {
        model: ScreenshotsModel {
            id: screenshotsModel
        }

        delegate: Image {
            source: small_image_url
            Layout.preferredHeight: root.side
            Layout.preferredWidth: root.side
            fillMode: Image.PreserveAspectFit
            smooth: true
            MouseArea {
                anchors.fill: parent
                onClicked: {
                    overlayImage.source = large_image_url
                    overlay.open()
                }
            }
        }
    }

    Behavior on opacity { NumberAnimation { easing.type: Easing.OutQuad; duration: 500 } }
}
