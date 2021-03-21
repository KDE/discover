/*
 *   SPDX-FileCopyrightText: 2012 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *   SPDX-FileCopyrightText: 2020 Carl Schwan <carl@carlschwan.eu>
 *
 *   SPDX-License-Identifier: LGPL-2.0-or-later
 */


import QtQuick 2.15
import QtQuick.Layouts 1.1
import QtQuick.Controls 2.1
import org.kde.discover 2.0
import org.kde.kirigami 2.14 as Kirigami

ListView {
    id: root
    readonly property alias count: screenshotsModel.count
    property alias resource: screenshotsModel.application
    property var resource

    spacing: Kirigami.Units.largeSpacing
    focus: overlay.visible
    orientation: Qt.Horizontal

    Layout.preferredHeight: Kirigami.Units.gridUnit * 10

    Keys.onLeftPressed:  if (leftAction.visible)  leftAction.trigger()
    Keys.onRightPressed: if (rightAction.visible) rightAction.trigger()

    model: ScreenshotsModel {
        id: screenshotsModel
    }

    delegate: AbstractButton {
        readonly property url imageSource: large_image_url
        readonly property real proportion: thumbnail.sourceSize.width>1 ? thumbnail.sourceSize.height/thumbnail.sourceSize.width : 1

        implicitWidth: thumbnail.width
        implicitHeight: root.height
        padding: Kirigami.Units.largeSpacing
        hoverEnabled: true
        onClicked: {
            root.currentIndex = model.row
            overlay.open()
        }

        HoverHandler {
            cursorShape: Qt.PointingHandCursor
        }

        background: Image {
            id: thumbnail
            readonly property real proportion: thumbnail.sourceSize.width>1 ? thumbnail.sourceSize.height/thumbnail.sourceSize.width : 1
            width: root.height / proportion
            height: root.height

            BusyIndicator {
                visible: running
                running: thumbnail.status == Image.Loading
                anchors.centerIn: parent
            }
            source: small_image_url
        }
    }

    Popup {
        id: overlay
        parent: applicationWindow().overlay
        modal: true
        clip: false

        x: (parent.width - width)/2
        y: (parent.height - height)/2
        readonly property real proportion: overlayImage.sourceSize.width>1 ? overlayImage.sourceSize.height/overlayImage.sourceSize.width : 1
        height: overlayImage.status == Image.Loading ? Kirigami.Units.gridUnit * 5 : Math.min(parent.height * 0.9, (parent.width * 0.9) * proportion, overlayImage.sourceSize.height)
        width: (height - 2 * padding)/proportion

        BusyIndicator {
            id: indicator
            visible: running
            running: overlayImage.status == Image.Loading
            anchors.fill: parent
        }

        Image {
            id: overlayImage
            anchors.fill: parent
            source: root.currentItem ? root.currentItem.imageSource : ""
            fillMode: Image.PreserveAspectFit
            smooth: true
        }

        Button {
            anchors {
                left: parent.right
                bottom: parent.top
            }
            icon.name: "window-close"
            onClicked: overlay.close()
        }


        RoundButton {
            anchors {
                right: parent.left
                verticalCenter: parent.verticalCenter
            }
            visible: leftAction.visible
            icon.name: leftAction.iconName
            onClicked: leftAction.triggered(null)
        }

        RoundButton {
            anchors {
                left: parent.right
                verticalCenter: parent.verticalCenter
            }
            visible: rightAction.visible
            icon.name: rightAction.iconName
            onClicked: rightAction.triggered(null)
        }

        Kirigami.Action {
            id: leftAction
            icon.name: "arrow-left"
            enabled: overlay.visible && visible
            visible: root.currentIndex >= 1 && !indicator.running
            onTriggered: root.currentIndex = (root.currentIndex - 1) % screenshotsModel.count
        }

        Kirigami.Action {
            id: rightAction
            icon.name: "arrow-right"
            enabled: overlay.visible && visible
            visible: root.currentIndex < (root.count - 1) && !indicator.running
            onTriggered: root.currentIndex = (root.currentIndex + 1) % screenshotsModel.count
        }
    }


    clip: true

    Shadow {
        parent: root
        anchors {
            left: parent.left
            top: parent.top
            bottom: parent.bottom
        }
        edge: Qt.LeftEdge
        width: Math.max(0, Math.min(root.width/5, root.contentX))
    }

    Shadow {
        parent: root
        anchors {
            right: parent.right
            top: parent.top
            bottom: parent.bottom
        }
        edge: Qt.RightEdge
        width: Math.max(0, Math.min(root.contentWidth - root.contentX - root.width)/5)
    }

    RoundButton {
        anchors {
            left: parent.left
            leftMargin: Kirigami.Units.largeSpacing
            verticalCenter: parent.verticalCenter
        }
        width: Kirigami.Units.gridUnit * 2
        height: width
        icon.name: "arrow-left"
        visible: !Kirigami.Settings.isMobile && root.currentIndex > 0
        Keys.forwardTo: [root]
        onClicked: root.currentIndex -= 1
    }

    RoundButton {
        anchors {
            right: parent.right
            rightMargin: Kirigami.Units.largeSpacing
            verticalCenter: parent.verticalCenter
        }
        width: Kirigami.Units.gridUnit * 2
        height: width
        icon.name: "arrow-right"
        visible: !Kirigami.Settings.isMobile && root.currentIndex < root.count - 1
        Keys.forwardTo: [root]
        onClicked: root.currentIndex += 1
    }
}
