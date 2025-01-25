/*
 *   SPDX-FileCopyrightText: 2023 ivan tkachenko <me@ratijas.tk>
 *
 *   SPDX-License-Identifier: LGPL-2.0-or-later
 */

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Templates as T
import Qt5Compat.GraphicalEffects as GE
import org.kde.kirigami as Kirigami

Item {
    id: delegate

    signal activated()

    required property int index

    required property url small_image_url
    required property url large_image_url
    required property size thumbnailSize

    readonly property url smallImageUrlIfNeeded: {
        if (small_image_url.toString() !== ""
            && (!loadLargeImage
                || !controlRoot.largeImageView
                || controlRoot.largeImageView.status !== Image.Ready)) {
            return thumbnailSize.height >= smallImageLoader.height ? small_image_url : large_image_url;
        }
        return "";
    }

    required property bool isAnimated
    readonly property bool isProbablyAnimated: isAnimated || large_image_url.toString().endsWith(".gif")

    property bool dim: true

    property bool loadLargeImage: false

    readonly property real widestRatio: 2/1

    anchors.verticalCenter: parent?.verticalCenter
    height: {
        if (!ListView.view) {
            return 0;
        }
        const widthForWidestRatio = Math.round(ListView.view.height * widestRatio);
        if (widthForWidestRatio > ListView.view.width) {
            const ratio = widthForWidestRatio / ListView.view.width;
            return Math.floor(ListView.view.height / ratio);
        } else {
            return ListView.view.height;
        }
    }
    width: Math.round(height * widestRatio)

    readonly property bool isCurrentItem: ListView.isCurrentItem

    readonly property alias activeImage: controlRoot.activeImage
    readonly property alias activeAnimatedImage: controlRoot.activeAnimatedImage

    QQC2.AbstractButton {
        id: controlRoot

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

        readonly property bool smallImageFailed: smallImageView !== null && smallImageView.status === Image.Error

        // Purely for graphical purposes, in case small image is not
        // available. Does not affect playback behavior.
        readonly property bool effectiveLoadLargeImage: delegate.loadLargeImage || smallImageFailed

        readonly property Image activeImage: effectiveLoadLargeImage ? largeImageView : smallImageView

        readonly property AnimatedImage activeAnimatedImage: activeImage as AnimatedImage

        readonly property Image largeImageView: largeImageLoader.item
        readonly property Image smallImageView: smallImageLoader.item

        width: Math.round(height * ratio)
        height: parent.height - backgroundShadow.shadow.size

        anchors.centerIn: parent
        anchors.verticalCenterOffset: -backgroundShadow.shadow.yOffset

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
                id: smallImageLoader

                anchors.fill: parent
                z: 1

                condition: delegate.isProbablyAnimated

                componentTrue: AnimatedImage {
                    fillMode: Image.PreserveAspectFit
                    source: delegate.smallImageUrlIfNeeded

                    playing: true
                    paused: true
                }

                componentFalse: Image {
                    fillMode: Image.PreserveAspectFit
                    source: delegate.smallImageUrlIfNeeded
                }
            }

            ConditionalLoader {
                id: largeImageLoader

                anchors.fill: parent
                z: 1

                active: controlRoot.effectiveLoadLargeImage
                condition: delegate.isProbablyAnimated

                componentTrue: AnimatedImage {
                    fillMode: Image.PreserveAspectFit
                    source: delegate.large_image_url

                    playing: true
                    paused: true
                }

                componentFalse: Image {
                    fillMode: Image.PreserveAspectFit
                    source: delegate.large_image_url
                }
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

                display: T.AbstractButton.IconOnly
                action: QQC2.Action {
                    text: {
                        const player = delegate.activeAnimatedImage;
                        if (!player) {
                            return "";
                        }
                        if (player.paused) {
                            return i18nc("@action:button Start playing media", "Play");
                        } else {
                            return i18nc("@action:button Pause any media that is playing", "Pause");
                        }
                    }
                    icon.name: {
                        const player = delegate.activeAnimatedImage;
                        if (!player) {
                            return "";
                        }
                        if (player.paused) {
                            return mirrored ? "media-playback-start-rtl-symbolic" : "media-playback-start-symbolic";
                        } else {
                            return mirrored ? "media-playback-pause-rtl-symbolic" : "media-playback-pause-symbolic";
                        }
                    }
                    enabled: delegate.activeAnimatedImage && delegate.isCurrentItem
                    shortcut: "Space"
                }
                // other icon properties are not bound automatically because RaoundButton overrides them
                icon.width: Kirigami.Units.iconSizes.large
                icon.height: Kirigami.Units.iconSizes.large
                icon.color: "white"

                transform: []
                background: Rectangle {
                    radius: width

                    border.width: 3
                    border.color: "white"
                    color: Qt.rgba(0, 0, 0, playPauseButton.down ? 0.5 : 0.3)
                }

                visible: delegate.isProbablyAnimated && opacity !== 0

                function show(animated: bool, autohide: bool) {
                    autohidePlayPauseButtonTimer.stop();
                    hidePlayPauseAnimator.stop();
                    if (animated) {
                        showPlayPauseAnimator.start();
                    } else {
                        showPlayPauseAnimator.stop();
                        opacity = 1;
                    }
                    if (autohide) {
                        this.autohide();
                    }
                }

                function hide(animated: bool) {
                    autohidePlayPauseButtonTimer.stop();
                    showPlayPauseAnimator.stop();
                    if (animated) {
                        hidePlayPauseAnimator.start();
                    } else {
                        hidePlayPauseAnimator.stop();
                        opacity = 0;
                    }
                }

                function autohide() {
                    autohidePlayPauseButtonTimer.restart();
                }

                function play(animated: bool) {
                    const player = delegate.activeAnimatedImage;
                    if (!player) {
                        return;
                    }
                    player.paused = false;
                    if (animated) {
                        autohide();
                    } else {
                        hide(false);
                    }
                }

                function pause(animated: bool) {
                    const player = delegate.activeAnimatedImage;
                    if (!player) {
                        return;
                    }
                    player.paused = true;
                    show(animated, false);
                }

                function toggle(animated: bool) {
                    const player = delegate.activeAnimatedImage;
                    if (!player) {
                        return;
                    }
                    if (player.paused) {
                        play(animated);
                    } else {
                        pause(animated);
                    }
                }

                function toggleOrActivate() {
                    if (delegate.loadLargeImage && delegate.activeAnimatedImage) {
                        toggle(true);
                    } else {
                        delegate.activated();
                    }
                }

                Timer {
                    id: autohidePlayPauseButtonTimer
                    interval: Kirigami.Units.humanMoment
                    running: false
                    onTriggered: {
                        playPauseButton.hide(true);
                    }
                }

                OpacityAnimator {
                    id: showPlayPauseAnimator
                    target: playPauseButton
                    to: 1
                    running: false
                    duration: Kirigami.Units.longDuration
                    easing.type: Easing.InOutQuad
                }

                OpacityAnimator {
                    id: hidePlayPauseAnimator
                    target: playPauseButton
                    to: 0
                    running: false
                    duration: Kirigami.Units.shortDuration
                    easing.type: Easing.InOutQuad
                }

                onClicked: {
                    toggleOrActivate();
                }
            }
        }

        background: Kirigami.ShadowedRectangle {
            id: backgroundShadow

            color: "transparent"
            radius: controlRoot.radius

            shadow.size: 20
            shadow.xOffset: 0
            shadow.yOffset: 5
            shadow.color: Qt.rgba(0, 0, 0, 0.4)
        }

        onClicked: {
            playPauseButton.toggleOrActivate();
        }

        MouseArea {
            anchors.fill: parent
            z: -1
            hoverEnabled: !Kirigami.Settings.hasTransientTouchInput
            visible: delegate.activeAnimatedImage && delegate.loadLargeImage && delegate.isCurrentItem
            onPositionChanged: mouse => {
                playPauseButton.show(/*animated*/true, /*autohide*/true);
            }
        }
    }

    function play() {
        if (loadLargeImage) {
            playPauseButton.play(/*animated*/true);
        }
    }

    function __initPlayPause() {
        if (activeAnimatedImage) {
            playPauseButton.show(/*animated*/false, /*autohide*/false);
        } else {
            playPauseButton.hide(/*animated*/false);
        }
    }

    onIsCurrentItemChanged: {
        if (!isCurrentItem) {
            playPauseButton.pause(true);
        }
    }

    Component.onCompleted: {
        __initPlayPause();
    }
}
