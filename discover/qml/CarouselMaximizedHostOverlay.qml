/*
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

QQC2.Popup {
    id: popup

    required property ScreenshotsModel carouselModel
    required property int currentIndex

    property url small_image_url
    property url large_image_url
    property bool isAnimated

    Component.onCompleted: update()
    onCurrentIndexChanged: update()

    function update() {
        const index = carouselModel.index(currentIndex, 0);
        small_image_url = carouselModel.data(index, ScreenshotsModel.ThumbnailUrl);
        large_image_url = carouselModel.data(index, ScreenshotsModel.ScreenshotUrl);
        isAnimated = carouselModel.data(index, ScreenshotsModel.IsAnimatedRole);
    }

    anchors.centerIn: parent
    parent: applicationWindow().overlay
    z: applicationWindow().globalDrawer.z + 10

    closePolicy: QQC2.Popup.CloseOnEscape
    visible: false
    focus: true
    modal: true
    dim: true

    width: parent?.Window.window?.width ?? 0
    height: parent?.Window.window?.height ?? 0

    topMargin: 0
    leftMargin: 0
    rightMargin: 0
    bottomMargin: 0

    topPadding: 0
    leftPadding: 0
    rightPadding: 0
    bottomPadding: 0

    contentItem: Item {

        Kirigami.Theme.inherit: true
        Kirigami.Theme.textColor: "white"

        TapHandler {
            onTapped: popup.close();
        }

        Item {
            id: carouselContainer

            anchors {
                fill: parent
                margins: Kirigami.Units.gridUnit * 7
            }

            CarouselDelegate {
                id: delegate

                readonly property size targetSize: {
                    if (implicitWidth < parent.width && implicitHeight < parent.height) {
                        return Qt.size(
                            parent.width + leftPadding + rightPadding,
                            parent.height + topPadding + bottomPadding,
                        );
                    }
                    const preferredWidth = implicitHeight * preferredRatio;
                    const preferredHeight = implicitWidth / preferredRatio;

                    let scale = 1.0;
                    if (preferredWidth > parent.width) {
                        scale = Math.min(scale, parent.width / preferredWidth);
                    }
                    if (preferredHeight > parent.height) {
                        scale = Math.min(scale, parent.height / preferredHeight);
                    }
                    return Qt.size(
                        Math.round(preferredWidth * scale) + leftPadding + rightPadding,
                        Math.round(preferredHeight * scale) + topPadding + bottomPadding,
                    );
                }

                readonly property point targetTopLeft: Qt.point(
                    Math.round((parent.width - targetSize.width) / 2),
                    Math.round((parent.height - targetSize.height) / 2),
                )

                readonly property point targetBottomRight: Qt.point(
                    targetTopLeft.x + targetSize.width,
                    targetTopLeft.y + targetSize.height,
                )

                x: targetTopLeft.x
                y: targetTopLeft.y

                width: targetBottomRight.x + leftPadding + rightPadding - x
                height: targetBottomRight.y + topPadding + bottomPadding - y

                dim: false
                loadLarge: true

                index: popup.currentIndex
                small_image_url: popup.small_image_url
                large_image_url: popup.large_image_url
                isAnimated: popup.isAnimated

                onActivated: popup.close()
            }
        }
    }

    background: Item {
        Rectangle {
            anchors.fill: parent
            color: "#262828"
        }
    }

    QQC2.Overlay.modal: Item { }

    onClosed: root.closed()

    onAboutToHide: {
        refreshOrigin();
    }
}
