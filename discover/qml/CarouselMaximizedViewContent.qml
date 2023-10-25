/*
 *   SPDX-FileCopyrightText: 2023 ivan tkachenko <me@ratijas.tk>
 *
 *   SPDX-License-Identifier: LGPL-2.0-or-later
 */

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import QtQuick.Templates as T
import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.components as KirigamiComponents
import org.kde.discover

Item {
    id: root

    required property CarouselAbstractMaximizedView host

    readonly property real edgeMargin: Kirigami.Units.gridUnit

    readonly property real windowControlButtonsCollapsedMargin: host.toggleModeAvailable
        ? Math.round(edgeMargin / 2) : edgeMargin

    Kirigami.Theme.colorSet: Kirigami.Theme.Complementary
    Kirigami.Theme.inherit: false

    RowLayout {
        z: 2
        anchors.top: parent.top
        anchors.right: parent.right
        spacing: 0

        KirigamiComponents.FloatingButton {
            Kirigami.Theme.inherit: true

            text: {
                switch (root.host.mode) {
                case CarouselMaximizedViewController.Mode.FullScreen:
                    return i18nc("@action:button", "Switch to Overlay");
                case CarouselMaximizedViewController.Mode.Overlay:
                    return i18nc("@action:button", "Switch to Full Screen");
                }
                return "";
            }
            icon.name: {
                switch (root.host.mode) {
                case CarouselMaximizedViewController.Mode.FullScreen:
                    return "window-restore-symbolic";
                case CarouselMaximizedViewController.Mode.Overlay:
                    return "window-maximize-symbolic";
                }
                return "";
            }
            display: T.AbstractButton.IconOnly
            focusPolicy: Qt.NoFocus
            radius: Infinity

            margins: root.edgeMargin
            leftMargin: !mirrored ? margins : windowControlButtonsCollapsedMargin
            rightMargin: mirrored ? margins : windowControlButtonsCollapsedMargin

            visible: root.host.toggleModeAvailable

            QQC2.ToolTip.text: text
            QQC2.ToolTip.visible: (Kirigami.Settings.tabletMode ? pressed : hovered) && text !== ""
            QQC2.ToolTip.delay: Kirigami.Settings.tabletMode ? Qt.styleHints.mousePressAndHoldInterval : Kirigami.Units.toolTipDelay

            onClicked: root.host.toggleMode()
        }

        KirigamiComponents.FloatingButton {
            Kirigami.Theme.inherit: true

            text: i18nc("@action:button Close overlay/window/popup with carousel of screenshots", "Close")
            icon.name: "window-close-symbolic"
            display: T.AbstractButton.IconOnly
            focusPolicy: Qt.NoFocus
            radius: Infinity

            margins: root.edgeMargin
            leftMargin: mirrored ? margins : windowControlButtonsCollapsedMargin
            rightMargin: !mirrored ? margins : windowControlButtonsCollapsedMargin

            QQC2.ToolTip.text: text
            QQC2.ToolTip.visible: (Kirigami.Settings.tabletMode ? pressed : hovered) && text !== ""
            QQC2.ToolTip.delay: Kirigami.Settings.tabletMode ? Qt.styleHints.mousePressAndHoldInterval : Kirigami.Units.toolTipDelay

            onClicked: root.host.close(true);
        }
    }

    Keys.forwardTo: view

    ColumnLayout {
        z: 1
        anchors.fill: parent
        spacing: 0

        ListView {
            id: view

            Layout.fillWidth: true
            Layout.fillHeight: true

            keyNavigationEnabled: true
            interactive: true

            clip: true
            pixelAligned: true
            orientation: ListView.Horizontal
            snapMode: ListView.SnapToItem
            highlightRangeMode: ListView.StrictlyEnforceRange

            displayMarginBeginning: root.edgeMargin
            displayMarginEnd: root.edgeMargin

            preferredHighlightBegin: currentItem ? Math.round((width - currentItem.width) / 2) : 0
            preferredHighlightEnd: currentItem ? preferredHighlightBegin + currentItem.width : 0

            highlightMoveDuration: Kirigami.Units.longDuration
            highlightResizeDuration: Kirigami.Units.longDuration

            spacing: Kirigami.Units.gridUnit
            cacheBuffer: 10000

            Component.onCompleted: {
                // HACK: Layout polish loop detected for QQuickColumnLayout. Aborting after two iterations.
                //
                // That error leads to ListView content being initially mis-positioned.
                // So, let's fire a callback exactly after two event loop iterations.
                Qt.callLater(() => {
                    Qt.callLater(() => {
                        // Set current index without animations
                        const backup = highlightMoveDuration;
                        highlightMoveDuration = 0;

                        model = Qt.binding(() => root.host.model);
                        currentIndex = Qt.binding(() => root.host.currentIndex);
                        currentIndexChanged.connect(() => root.host.currentIndex = currentIndex);
                        // Autoplay current video when view is first opened.
                        currentItem?.play();

                        highlightMoveDuration = backup;
                    });
                });
            }

            delegate: CarouselDelegate {
                z: 1
                loadLargeImage: true
                onActivated: {
                    if (root.host.currentIndex === index) {
                        root.host.close(true);
                    } else {
                        root.host.currentIndex = index;
                    }
                }
            }

            CarouselNavigationButtonsListViewAdapter {
                LayoutMirroring.enabled: root.LayoutMirroring.enabled

                view: view
                edgeMargin: root.edgeMargin
            }

            TapHandler {
                onTapped: {
                    root.host.close(true);
                }
            }
        }

        CarouselPageIndicator {
            id: pageIndicator

            Layout.fillWidth: true
            topPadding: Kirigami.Units.largeSpacing * 3
            bottomPadding: Kirigami.Units.largeSpacing * 3

            focusPolicy: Qt.NoFocus
            interactive: true
            count: carouselModel.count
            visible: count > 1

            function bindCurrentIndex() {
                currentIndex = Qt.binding(() => root.host.currentIndex);
            }

            onCurrentIndexChanged: {
                root.host.currentIndex = currentIndex;
            }
        }
    }
}
