/*
 *   SPDX-FileCopyrightText: 2012 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *   SPDX-FileCopyrightText: 2020 Carl Schwan <carl@carlschwan.eu>
 *
 *   SPDX-License-Identifier: LGPL-2.0-or-later
 */


import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15
import org.kde.discover 2.0
import org.kde.kirigami 2.19 as Kirigami

ListView {
    id: root
    readonly property alias count: screenshotsModel.count
    property alias resource: screenshotsModel.application
    property var resource

    implicitHeight: Kirigami.Units.gridUnit * 20
    implicitWidth: contentWidth
    contentWidth: contentItem.childrenRect.width
    spacing: Kirigami.Units.largeSpacing
    orientation: Qt.Horizontal
    boundsBehavior: Flickable.StopAtBounds
    // 1.8 is probably good enough for approximating the aspect ratio of most screenshots
    highlightMoveVelocity: (height * 1.8) / (Kirigami.Units.longDuration / 1000)
    highlightResizeDuration: 0
    currentIndex: 0
    interactive: contentWidth > width
    pixelAligned: true
    // keep images in memory to smooth out horizontal scrolling
    cacheBuffer: Math.min(count, 10)
    // Load some images early to smooth out horizontal scrolling
    // and also to allow `itemAt()` and `indexAt()` to work.
    // A little more than the approximation above just to make it
    // more likely to get at least 1 extra image to the left and right.
    displayMarginBeginning: height * 2
    displayMarginEnd: displayMarginBeginning

    Accessible.role: Accessible.List
    Accessible.name: i18nc("list view", "Screenshots")

    Kirigami.WheelHandler {
        id: wheelHandler
        target: root
        filterMouseEvents: true
    }

    model: ScreenshotsModel {
        id: screenshotsModel
    }

    // Using a MouseArea instead of a TapHandler because *Handlers over
    // a ListView sometimes allow the ListView to be flicked with a mouse.
    delegate: MouseArea {
        id: delegate
        enabled: !overlay.visible
        cursorShape: enabled ? Qt.PointingHandCursor : Qt.ArrowCursor
        height: root.height
        implicitWidth: if (thumbnail.status == Image.Loading) {
            // 1.8 is probably good enough for approximating the aspect ratio of most screenshots
            Math.floor(height * 1.8)
        } else {
            thumbnail.implicitWidth
        }
        Accessible.role: Accessible.ListItem
        Image {
            id: thumbnail
            asynchronous: true
            smooth: false
            source: small_image_url
            // QQuickImage::updatePaintedGeometry() sets implicitWidth to paintedWidth
            // when using PreserveAspectFit and height is explicitly set but width is not.
            // The implicit size of Image cannot be directly set in QML.
            fillMode: Image.PreserveAspectFit
            // Using fixed source size so that the images don't keep reloading when the window is being resized
            sourceSize.height: Kirigami.Units.gridUnit * 20
            height: parent.height
            width: undefined
            Accessible.role: Accessible.Graphic
        }
        BusyIndicator {
            visible: running
            running: thumbnail.status == Image.Loading
            anchors.centerIn: parent
        }
        onClicked: {
            overlay.currentIndex = model.row
            overlay.open()
            overlay.forceActiveFocus()
        }
    }

    Action {
        id: leftListAction
        enabled: !root.atXBeginning
        onTriggered: {
            // Try to get an index at the edge or just outside of the view
            let targetX = root.contentX
            let targetIndex = root.indexAt(targetX, 0)
            if (targetIndex === -1) {
                targetIndex = root.indexAt(targetX - root.spacing, 0)
            }
            if (targetIndex === -1) {
                targetIndex = root.indexAt(targetX + root.spacing, 0)
            }
            const oldIndex = root.currentIndex
            root.currentIndex = targetIndex
            if (oldIndex === targetIndex) {
                root.decrementCurrentIndex()
            }
        }
    }

    Action {
        id: rightListAction
        enabled: !root.atXEnd
        onTriggered: {
            // Try to get an index at the edge or just outside of the view
            let targetX = root.contentX + root.width
            let targetIndex = root.indexAt(targetX, 0)
            if (targetIndex === -1) {
                targetIndex = root.indexAt(targetX + root.spacing, 0)
            }
            if (targetIndex === -1) {
                targetIndex = root.indexAt(targetX - root.spacing, 0)
            }
            const oldIndex = root.currentIndex
            root.currentIndex = targetIndex
            if (oldIndex === targetIndex) {
                root.incrementCurrentIndex()
            }
        }
    }

    RoundButton {
        id: leftButton
        parent: root
        anchors {
            left: parent.left
            leftMargin: Kirigami.Units.largeSpacing
            verticalCenter: parent.verticalCenter
        }
        action: leftListAction
        icon.name: mirrored ? "arrow-right" : "arrow-left"
        width: Kirigami.Units.gridUnit * 2
        height: width
        visible: !Kirigami.Settings.isMobile && enabled && !overlay.visible
    }

    RoundButton {
        id: rightButton
        parent: root
        anchors {
            right: parent.right
            rightMargin: Kirigami.Units.largeSpacing
            verticalCenter: parent.verticalCenter
        }
        action: rightListAction
        icon.name: mirrored ? "arrow-left" : "arrow-right"
        width: Kirigami.Units.gridUnit * 2
        height: width
        visible: !Kirigami.Settings.isMobile && enabled && !overlay.visible
    }

    Keys.onLeftPressed: LayoutMirroring.enabled ? rightListAction.trigger() : leftListAction.trigger()
    Keys.onRightPressed: LayoutMirroring.enabled ? leftListAction.trigger() : rightListAction.trigger()

    Popup {
        id: overlay
        property int currentIndex: 0
        parent: Overlay.overlay
        modal: true
        clip: false

        x: Math.round((parent.width - width)/2)
        y: Math.round((parent.height - height)/2)

        width: parent.width - leftMargin - rightMargin
        height: parent.height - topMargin - bottomMargin
        margins: Kirigami.Units.gridUnit * 2 + closeButton.implicitWidth
        padding: Kirigami.Units.gridUnit

        // Using MouseArea because it's a bit unpredictable when something below the popup will also accept taps from TapHandler
        MouseArea {
            // The parent of contentItem is a Page
            parent: overlay.contentItem.parent
            anchors.fill: parent
            onClicked: overlay.close()
        }

        contentItem: Loader {
            id: imageLoader
            focus: visible
            active: visible
            sourceComponent: Image {
                asynchronous: true
                source: {
                    const index = screenshotsModel.index(overlay.currentIndex, 0)
                    return screenshotsModel.data(index, Qt.UserRole + 2)
                }
                // TODO: set sourceSize so that images are always smooth.
                // The `smooth` property (on by default) is not enough.
                fillMode: Image.PreserveAspectFit
                BusyIndicator {
                    id: indicator
                    visible: running
                    running: parent.status == Image.Loading
                    anchors.centerIn: parent

                    background: Rectangle {
                        color: Kirigami.Theme.backgroundColor
                        radius: height / 2
                        opacity: indicator.running
                        Behavior on opacity {
                            OpacityAnimator { duration: Kirigami.Units.longDuration }
                        }
                    }
                }
            }

            Keys.onLeftPressed: mirrored ? rightPopupAction.trigger() : leftPopupAction.trigger()
            Keys.onRightPressed: mirrored ? leftPopupAction.trigger() : rightPopupAction.trigger()
        }

        background: null

        Action {
            id: leftPopupAction
            enabled: overlay.currentIndex > 0
            onTriggered: overlay.currentIndex = Math.max(0, overlay.currentIndex - 1)
        }

        Action {
            id: rightPopupAction
            enabled: overlay.currentIndex < root.count - 1
            onTriggered: overlay.currentIndex = Math.min(overlay.currentIndex + 1, root.count - 1)
        }

        RoundButton {
            id: closeButton
            parent: overlay.contentItem.parent
            anchors {
                left: parent.right
                bottom: parent.top
            }
            icon.name: "window-close"
            onClicked: overlay.close()
        }

        RoundButton {
            id: leftPopupButton
            parent: overlay.contentItem.parent
            anchors {
                right: parent.left
                verticalCenter: parent.verticalCenter
            }
            action: leftPopupAction
            visible: enabled
            icon.name: mirrored ? "arrow-right" : "arrow-left"
        }

        RoundButton {
            id: rightPopupButton
            parent: overlay.contentItem.parent
            anchors {
                left: parent.right
                verticalCenter: parent.verticalCenter
            }
            action: rightPopupAction
            visible: enabled
            icon.name: mirrored ? "arrow-left" : "arrow-right"
        }

        Overlay.modal: Rectangle {
            color: Qt.rgba(0,0,0,0.5)
        }

        onClosed: root.forceActiveFocus()
    }
}
