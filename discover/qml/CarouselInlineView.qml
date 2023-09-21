/*
 *   SPDX-FileCopyrightText: 2012 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *   SPDX-FileCopyrightText: 2020 Carl Schwan <carl@carlschwan.eu>
 *   SPDX-FileCopyrightText: 2023 ivan tkachenko <me@ratijas.tk>
 *
 *   SPDX-License-Identifier: LGPL-2.0-or-later
 */

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import QtQuick.Templates as T
import org.kde.discover
import org.kde.kirigami 2 as Kirigami

ListView {
    id: root

    required property ScreenshotsModel screenshotsModel

    property bool showNavigationArrows: true
    property int failedCount: 0
    readonly property bool hasFailed: count !== 0 && failedCount === count

    spacing: Kirigami.Units.largeSpacing
    focus: overlay.visible
    orientation: Qt.Horizontal
    cacheBuffer: 10 // keep some screenshots in memory

    Keys.onLeftPressed:  if (leftAction.visible)  leftAction.trigger()
    Keys.onRightPressed: if (rightAction.visible) rightAction.trigger()

    property real delegateHeight: height - topMargin - bottomMargin

    model: screenshotsModel

    delegate: QQC2.AbstractButton {
        readonly property bool animated: isAnimated
        readonly property url imageSource: large_image_url
        readonly property real proportion: (thumbnail.imageStatus === Image.Ready && thumbnail.sourceSize.width > 1)
            ? (thumbnail.sourceSize.height / thumbnail.sourceSize.width) : 1

        implicitWidth: root.delegateHeight / proportion
        implicitHeight: root.delegateHeight
        opacity: hovered ? 0.7 : 1

        hoverEnabled: true
        onClicked: {
            root.currentIndex = model.row
            overlay.open()
        }

        HoverHandler {
            cursorShape: Qt.PointingHandCursor
        }

        background: Item {
            QQC2.BusyIndicator {
                visible: running
                running: thumbnail.imageStatus === Image.Loading
                anchors.centerIn: parent
            }
            Kirigami.Icon {
                implicitWidth: Kirigami.Units.iconSizes.large
                implicitHeight: Kirigami.Units.iconSizes.large
                visible: thumbnail.imageStatus === Image.Error
                source: "image-missing"
            }
            ConditionalLoader {
                id: thumbnail
                anchors.fill: parent
                readonly property int imageStatus: item.status
                readonly property var sourceSize: item.sourceSize
                condition: isAnimated

                componentFalse: Component {
                    Image {
                        source: small_image_url

                        onStatusChanged: {
                            if (status == Image.Error) {
                                source = large_image_url
                            }
                        }
                    }
                }
                componentTrue: Component {
                    AnimatedImage {
                        source: small_image_url

                        onStatusChanged: {
                            if (status == Image.Error) {
                                source = large_image_url
                            }
                        }
                    }
                }

                onImageStatusChanged: {
                    if (imageStatus === Image.Error) {
                        root.failedCount += 1;
                    }
                }
            }
        }
    }

    CarouselNavigationButtonsListViewAdapter {
        LayoutMirroring.enabled: root.LayoutMirroring.enabled

        view: root
        policy: CarouselNavigationButtonsListViewAdapter.CurrentIndex
        edgeMargin: Kirigami.Units.largeSpacing
    }

    QQC2.Popup {
        id: overlay
        parent: applicationWindow().overlay
        z: applicationWindow().globalDrawer.z + 10
        modal: true
        clip: false

        x: (parent.width - width) / 2
        y: (parent.height - height) / 2
        readonly property real proportion: (overlayImage.sourceSize.width > 1) ? (overlayImage.sourceSize.height / overlayImage.sourceSize.width) : 1
        height: overlayImage.imageStatus >= Image.Loading ? Kirigami.Units.gridUnit * 20 : Math.min(parent.height * 0.9, (parent.width * 0.9) * proportion, overlayImage.sourceSize.height)
        width: overlayImage.imageStatus >= Image.Loading ? Kirigami.Units.gridUnit * 20 : (height - 2 * padding) / proportion

        QQC2.BusyIndicator {
            id: indicator
            visible: running
            running: overlayImage.imageStatus === Image.Loading
            anchors.centerIn: parent
        }

        Kirigami.Icon {
            implicitWidth: Kirigami.Units.iconSizes.large
            implicitHeight: Kirigami.Units.iconSizes.large
            visible: overlayImage.imageStatus === Image.Error
            source: "image-missing"
        }

        ConditionalLoader {
            id: overlayImage
            anchors.fill: parent
            readonly property var imageStatus: item.status
            readonly property var sourceSize: item.sourceSize
            condition: root.currentItem?.animated ?? false

            componentFalse: Component {
                Image {
                    source: root.currentItem ? root.currentItem.imageSource : ""
                    fillMode: Image.PreserveAspectFit
                    smooth: true
                }
            }
            componentTrue: Component {
                AnimatedImage {
                    source: root.currentItem ? root.currentItem.imageSource : ""
                    fillMode: Image.PreserveAspectFit
                    smooth: true
                }
            }

            onImageStatusChanged: {
                if (imageStatus === Image.Error) {
                    root.failedCount += 1;
                }
            }
        }

        QQC2.Button {
            anchors {
                left: parent.right
                bottom: parent.top
            }
            icon.name: "window-close"
            onClicked: overlay.close()
        }

        CarouselNavigationButtonsListViewAdapter {
            LayoutMirroring.enabled: root.LayoutMirroring.enabled

            view: root
        }

        Kirigami.Action {
            id: leftAction
            icon.name: root.LayoutMirroring.enabled ? "arrow-right" : "arrow-left"
            enabled: overlay.visible && visible
            visible: root.currentIndex >= 1 && !indicator.running
            onTriggered: root.currentIndex = (root.currentIndex - 1) % root.count
        }

        Kirigami.Action {
            id: rightAction
            icon.name: root.LayoutMirroring.enabled ? "arrow-left" : "arrow-right"
            enabled: overlay.visible && visible
            visible: root.currentIndex < (root.count - 1) && !indicator.running
            onTriggered: root.currentIndex = (root.currentIndex + 1) % root.count
        }
    }

    clip: true
}
