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
import QtQuick.Controls 2.1 as QQC2
import org.kde.discover 2.0
import org.kde.kirigami 2.0 as Kirigami

Flow {
    id: root
    property alias resource: screenshotsModel.application

    spacing: Kirigami.Units.smallSpacing

    readonly property real side: Kirigami.Units.gridUnit * 8
    property QtObject page
    visible: screenshotsModel.count>0

    property int currentIndex: -1
    readonly property Item currentItem: root.currentIndex >= 0 ? rep.itemAt(root.currentIndex) : null

    Keys.onLeftPressed:  if (leftAction.visible)  leftAction.trigger()
    Keys.onRightPressed: if (rightAction.visible) rightAction.trigger()
    focus: true

    QQC2.Popup {
        id: overlay
        parent: applicationWindow().overlay
        modal: true

        x: (parent.width - width)/2
        y: (parent.height - height)/2
        height: Math.min(parent.height * 0.9, overlayImage.sourceSize.height)
        width: Math.min(parent.width * 0.9, overlayImage.sourceSize.width)

        Image {
            id: overlayImage
            anchors.fill: parent
            source: root.currentItem ? root.currentItem.imageSource : ""
            fillMode: Image.PreserveAspectFit
            smooth: true
        }

        Button {
            anchors {
                right: parent.left
                verticalCenter: parent.verticalCenter
            }
            visible: leftAction.visible
            iconName: leftAction.iconName
            onClicked: leftAction.triggered(null)
        }

        Button {
            anchors {
                left: parent.right
                verticalCenter: parent.verticalCenter
            }
            visible: rightAction.visible
            iconName: rightAction.iconName
            onClicked: rightAction.triggered(null)
        }

        Kirigami.Action {
            id: leftAction
            iconName: "arrow-left"
            enabled: overlay.visible && visible
            visible: root.currentIndex >= 1
            onTriggered: root.currentIndex -= 1
        }

        Kirigami.Action {
            id: rightAction
            iconName: "arrow-right"
            enabled: overlay.visible && visible
            visible: root.currentIndex < (rep.count - 1)
            onTriggered: root.currentIndex += 1
        }
    }

    Repeater {
        id: rep
        model: ScreenshotsModel {
            id: screenshotsModel
        }

        delegate: Image {
            source: small_image_url
            height: root.side
            width: root.side
            fillMode: Image.PreserveAspectCrop
            smooth: true
            opacity: mouse.containsMouse? 0.5 : 1
            readonly property url imageSource: large_image_url

            Behavior on opacity { NumberAnimation { easing.type: Easing.OutQuad; duration: 200 } }

            BusyIndicator {
                visible: running
                running: parent.status == Image.Loading
                anchors.centerIn: parent
            }

            MouseArea {
                id: mouse
                anchors.fill: parent
                hoverEnabled: true
                onClicked: {
                    root.currentIndex = index
                    overlay.open()
                }
            }
        }
    }

    Behavior on opacity { NumberAnimation { easing.type: Easing.OutQuad; duration: 500 } }
}
