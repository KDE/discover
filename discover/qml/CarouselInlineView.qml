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
import org.kde.discover as Discover
import org.kde.kirigami as Kirigami

T.Control {
    id: root

    required property Discover.ScreenshotsModel carouselModel

    property real edgeMargin: 0
    property alias view: view

    property int currentIndex: 0

    // TODO: Re-add tracking of failed images, store per-screenshot data in model
    property int failedCount: 0
    readonly property bool hasFailed: failedCount !== 0 && failedCount === view.count

    implicitWidth: 0
    implicitHeight: implicitContentHeight

    padding: 0
    topPadding: undefined
    leftPadding: undefined
    rightPadding: undefined
    bottomPadding: undefined
    verticalPadding: undefined
    horizontalPadding: undefined

    focusPolicy: Qt.StrongFocus
    Keys.forwardTo: view

    contentItem: ColumnLayout {
        spacing: 0

        LayoutMirroring.enabled: root.mirrored
        LayoutMirroring.childrenInherit: true

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

            currentIndex: root.currentIndex

            onCurrentIndexChanged: {
                root.currentIndex = currentIndex;
            }

            Component.onCompleted: {
                // HACK: Layout polish loop detected for QQuickColumnLayout. Aborting after two iterations.
                //
                // That error leads to ListView content being initially mis-positioned.
                // So, let's fire a callback exactly after two event loop iterations.
                Qt.callLater(() => {
                    Qt.callLater(() => {
                        model = Qt.binding(() => root.carouselModel);
                    });
                });
            }

            delegate: CarouselDelegate {
                onActivated: {
                    if (root.currentIndex === index) {
                        controller.open(root.Window.window, root.carouselModel, index);
                    } else {
                        root.currentIndex = index;
                    }
                }
            }

            CarouselNavigationButtonsListViewAdapter {
                LayoutMirroring.enabled: root.LayoutMirroring.enabled

                Kirigami.Theme.colorSet: Kirigami.Theme.Button

                view: view
                edgeMargin: root.edgeMargin
            }
        }

        CarouselPageIndicator {
            id: pageIndicator

            Layout.fillWidth: true
            topPadding: Kirigami.Units.largeSpacing * 3

            focusPolicy: Qt.NoFocus
            interactive: true
            count: carouselModel.count
            visible: count > 1

            function bindCurrentIndex() {
                currentIndex = Qt.binding(() => root.currentIndex);
            }

            onCurrentIndexChanged: {
                root.currentIndex = currentIndex;
            }
        }
    }

    CarouselMaximizedViewController {
        id: controller

        onCurrentIndexChanged: currentIndex => {
            // Set current index without animations
            const backup = view.highlightMoveDuration;
            view.highlightMoveDuration = 0;

            root.currentIndex = currentIndex;

            view.highlightMoveDuration = backup;
        }
    }
}
