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

QQC2.Control {
    id: root

    required property ScreenshotsModel carouselModel

    property alias displayMarginBeginning: view.displayMarginBeginning
    property alias displayMarginEnd: view.displayMarginEnd
    property alias view: view

    // TODO: Re-add tracking of failed images, store per-screenshot data in model
    property int failedCount: 0
    readonly property bool hasFailed: failedCount !== 0 && failedCount === view.count

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
        spacing: Kirigami.Units.largeSpacing * 3

        LayoutMirroring.enabled: root.mirrored

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

            preferredHighlightBegin: currentItem ? Math.round((width - currentItem.width) / 2) : 0
            preferredHighlightEnd: currentItem ? preferredHighlightBegin + currentItem.width : 0

            highlightMoveDuration: Kirigami.Units.longDuration
            highlightResizeDuration: Kirigami.Units.longDuration

            spacing: Kirigami.Units.gridUnit
            cacheBuffer: 10000

            onCurrentIndexChanged: {
                pageIndicator.currentIndex = currentIndex;
            }

            model: root.carouselModel

            delegate: CarouselDelegate {
                onActivated: {
                    if (ListView.view.currentIndex === index) {
                        maximizedHost.open(index);
                    } else {
                        ListView.view.currentIndex = index;
                    }
                }
            }

            CarouselNavigationButton {
                LayoutMirroring.enabled: root.mirrored
                anchors.left: parent.left
                height: parent.height
                view: view
                role: Qt.AlignLeading
                extraEdgePadding: root.displayMarginBeginning

                onClicked: {
                    view.decrementCurrentIndex();
                }
            }

            CarouselNavigationButton {
                LayoutMirroring.enabled: root.mirrored
                anchors.right: parent.right
                height: parent.height
                view: view
                role: Qt.AlignTrailing
                extraEdgePadding: root.displayMarginEnd

                onClicked: {
                    view.incrementCurrentIndex();
                }
            }
        }

        CarouselPageIndicator {
            id: pageIndicator

            Layout.fillWidth: true

            focusPolicy: Qt.NoFocus

            interactive: true
            count: view.count

            onCurrentIndexChanged: {
                view.currentIndex = currentIndex
            }
            Component.onCompleted: {
                currentIndex = view.currentIndex;
            }
        }
    }

    CarouselMaximizedHostOverlay {
        id: maximizedHost

        carouselModel: root.carouselModel
        currentIndex: view.currentIndex
    }
}
