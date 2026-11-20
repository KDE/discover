// SPDX-FileCopyrightText: 2026 Devin Lin <devin@kde.org>
// SPDX-License-Identifier: GPL-2.0-or-later

pragma ComponentBehavior: Bound

import QtQuick

import org.kde.kirigami as Kirigami
import org.kde.bigscreen.ui as BigscreenUI

BigscreenUI.Page {
    id: root

    property alias model: view.model
    property alias currentIndex: view.currentIndex

    BigscreenUI.BackHandler.onActivated: BigscreenUI.ApplicationWindow.window.pageStack.pop()

    background: Rectangle { color: "black" }

    ListView {
        id: view

        anchors.fill: parent
        focus: true
        orientation: ListView.Horizontal
        snapMode: ListView.SnapOneItem
        highlightRangeMode: ListView.StrictlyEnforceRange
        highlightMoveDuration: Kirigami.Units.longDuration
        keyNavigationEnabled: true
        boundsBehavior: Flickable.StopAtBounds
        clip: true

        delegate: Item {
            id: screenshotDelegate

            required property url large_image_url
            required property bool isAnimated

            width: view.width
            height: view.height

            Loader {
                id: imageLoader
                anchors.fill: parent
                anchors.margins: Kirigami.Units.gridUnit * 2
                sourceComponent: screenshotDelegate.isAnimated ? animatedImage : stillImage
            }

            Component {
                id: stillImage
                Image {
                    source: screenshotDelegate.large_image_url
                    fillMode: Image.PreserveAspectFit
                    asynchronous: true
                    smooth: true
                }
            }

            Component {
                id: animatedImage
                AnimatedImage {
                    source: screenshotDelegate.large_image_url
                    fillMode: Image.PreserveAspectFit
                    asynchronous: true
                    playing: screenshotDelegate.ListView.isCurrentItem
                }
            }

            BigscreenUI.BusyIndicator {
                anchors.centerIn: parent
                visible: (imageLoader.item?.status ?? Image.Loading) === Image.Loading
                running: visible
            }
        }
    }

    BigscreenUI.Badge {
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: Kirigami.Units.gridUnit
        visible: view.count > 1
        text: i18nc("%1 is the current screenshot, %2 how many there are", "%1 / %2", view.currentIndex + 1, view.count)
    }

    Component.onCompleted: view.positionViewAtIndex(view.currentIndex, ListView.Beginning)
}
