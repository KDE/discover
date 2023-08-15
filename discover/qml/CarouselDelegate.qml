/*
 *   SPDX-FileCopyrightText: 2023 ivan tkachenko <me@ratijas.tk>
 *
 *   SPDX-License-Identifier: LGPL-2.0-or-later
 */

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import Qt5Compat.GraphicalEffects as GE

import org.kde.kirigami 2 as Kirigami

Item {
    id: delegate

    signal activated()

    required property int index

    required property url small_image_url
    required property url large_image_url
    required property bool isAnimated

    property bool dim: true

    property bool loadLarge: false

    readonly property real widestRatio: 2/1

    height: ListView.view?.height ?? 0
    width: Math.round(height * widestRatio)

    readonly property bool isCurrentItem: ListView.isCurrentItem

    readonly property alias activeImage: controlRoot.activeImage

    QQC2.AbstractButton {
        id: controlRoot

        // property real radius: Kirigami.Units.smallSpacing
        property real radius: Kirigami.Units.largeSpacing

        readonly property real minimumRatio: 1/2
        readonly property real maximumRatio: 2/1

        readonly property real preferredRatio: {
            if (!activeImage || activeImage.status !== Image.Ready || activeImage.implicitHeight === 0) {
                return 3/2;
            }
            return activeImage.implicitWidth / activeImage.implicitHeight;
        }

        readonly property real ratio: {
            return Math.max(minimumRatio, Math.min(maximumRatio, preferredRatio));
        }

        readonly property Image activeImage: delegate.loadLarge ? imageFullView : imageThumbnailView

        readonly property alias imageFullView: imageFullView
        readonly property Image imageThumbnailView: imageThumbnailLoader.item

        width: Math.round(height * ratio)
        height: parent.height
        anchors.horizontalCenter: parent.horizontalCenter

        padding: 0
        topPadding: undefined
        leftPadding: undefined
        rightPadding: undefined
        bottomPadding: undefined
        verticalPadding: undefined
        horizontalPadding: undefined

        contentItem: Item {
            id: content

            implicitWidth: controlRoot.activeImage?.implicitWidth ?? 0
            implicitHeight: controlRoot.activeImage?.implicitHeight ?? 0

            width: content.width
            height: content.height

            layer.enabled: true
            layer.effect: GE.OpacityMask {
                maskSource: Rectangle {
                    width: content.width
                    height: content.height

                    color: "black"
                    radius: controlRoot.radius
                }
            }

            Rectangle {
                anchors.fill: parent
                z: 0

                color: Qt.tint(controlRoot.Kirigami.Theme.backgroundColor, Qt.alpha(controlRoot.Kirigami.Theme.textColor, 0.14))

                QQC2.BusyIndicator {
                    anchors.centerIn: parent
                    running: controlRoot.activeImage?.status === Image.Loading
                }

                Kirigami.Icon {
                    anchors.centerIn: parent
                    implicitWidth: Kirigami.Units.iconSizes.large
                    implicitHeight: Kirigami.Units.iconSizes.large
                    visible: controlRoot.activeImage?.status === Image.Error
                    source: "image-missing"
                }
            }

            ConditionalLoader {
                id: imageThumbnailLoader

                anchors.fill: parent
                z: 1

                condition: delegate.isAnimated

                componentTrue: AnimatedImage {
                    fillMode: Image.PreserveAspectFit
                    source: delegate.small_image_url && (!delegate.loadLarge || imageFullView.status !== Image.Ready)
                        ? delegate.small_image_url : ""

                    playing: false
                }

                componentFalse: Image {
                    fillMode: Image.PreserveAspectFit
                    source: delegate.small_image_url && (!delegate.loadLarge || imageFullView.status !== Image.Ready)
                        ? delegate.small_image_url : ""
                }
            }

            Image {
                id: imageFullView

                anchors.fill: parent
                z: 2

                fillMode: Image.PreserveAspectFit
                source: delegate.loadLarge ? delegate.large_image_url : ""
            }

            opacity: (!delegate.dim || delegate.isCurrentItem) ? 1 : controlRoot.hovered ? 0.8 : 0.66

            Behavior on opacity {
                NumberAnimation {
                    duration: Kirigami.Units.longDuration
                    easing.type: Easing.InOutCubic
                }
            }

            QQC2.RoundButton {
                id: playPauseButton

                anchors.centerIn: parent
                z: 100

                visible: delegate.isAnimated
                icon.name: (controlRoot.imageThumbnailView?.playing ?? false)
                    ? "media-playback-pause-symbolic"
                    : "media-playback-start-symbolic"
                icon.width: Kirigami.Units.iconSizes.large
                icon.height: Kirigami.Units.iconSizes.large
                icon.color: "white"

                background: Rectangle {
                    radius: width

                    border.width: 3
                    border.color: "white"
                    color: Qt.rgba(0, 0, 0, playPauseButton.down ? 0.5 : 0.3)
                }

                onClicked: {
                    delegate.activated();
                    // const player = controlRoot.imageThumbnailView;
                    // if (player) {
                    //     player.playing = !player.playing;
                    // }
                }
            }
        }

        background: Kirigami.ShadowedRectangle {
            color: "transparent"
            radius: controlRoot.radius

            shadow.size: 20
            shadow.xOffset: 0
            shadow.yOffset: 5
            shadow.color: Qt.rgba(0, 0, 0, 0.4)
        }

        onClicked: {
            delegate.activated();
        }
    }
}
