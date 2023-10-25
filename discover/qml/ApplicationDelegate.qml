/*
 *   SPDX-FileCopyrightText: 2012 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *   SPDX-FileCopyrightText: 2018-2021 Nate Graham <nate@kde.org>
 *   SPDX-FileCopyrightText: 2023 ivan tkachenko <me@ratijas.tk>
 *
 *   SPDX-License-Identifier: LGPL-2.0-or-later
 */

import QtQuick 2.15
import QtQuick.Controls 2.1
import QtQuick.Layouts 1.1
import QtQuick.Window 2.1
import "navigation.js" as Navigation
import org.kde.discover 2.0
import org.kde.kirigami 2.14 as Kirigami

BasicAbstractCard {
    id: root

    property alias application: installButton.application
    property bool compact: false
    property bool showRating: true
    property bool showSize: false

    readonly property bool appIsFromNonDefaultBackend: ResourcesModel.currentApplicationBackend !== application.backend && application.backend.hasApplications
    showClickFeedback: true

    function trigger() {
        ListView.currentIndex = index
        Navigation.openApplication(application)
    }
    padding: Kirigami.Units.largeSpacing * 2
    highlighted: ListView.isCurrentItem
    Keys.onReturnPressed: trigger()
    onClicked: trigger()

    content: Item {
        implicitHeight: Math.max(columnLayout.implicitHeight, resourceIconFrame.implicitHeight)

        // App icon, This Item is to simplify the alignment logic a lot
        Item {
            id: resourceIconFrame
            anchors {
                left: parent.left
                top: parent.top
                bottom: parent.bottom
            }
            clip:true
            width: height
            implicitHeight: (root.compact ? Kirigami.Units.iconSizes.large : Kirigami.Units.iconSizes.huge) + (Kirigami.Units.largeSpacing * 2)

            Kirigami.Icon {
                id: resourceIcon
                anchors {
                    fill: parent
                    margins: Kirigami.Units.largeSpacing
                }
                readonly property real contHeight: root.compact ? Kirigami.Units.iconSizes.large : Kirigami.Units.iconSizes.huge
                source: application.icon
                animated: false
                height: contHeight
                width: contHeight
            }
        }

        // Container for everything but the app icon
        ColumnLayout {
            id: columnLayout

            anchors {
                top: parent.top
                right: parent.right
                bottom: parent.bottom
                left: resourceIconFrame.right
                leftMargin: Kirigami.Units.largeSpacing * 2
            }
            spacing: 0

            // Container for app name and backend name labels
            RowLayout {
                spacing: Kirigami.Units.largeSpacing

                // App name label
                Kirigami.Heading {
                    id: head
                    Layout.fillWidth: true
                    // We want the heading visually top-aligned with the top margin, the icon background and that in general
                    // it's root.padding away from the border. We can't just align the label to the top for it because internally
                    // everything is aligned respecting the text boundingRect, which includes blank space on the top as a "line" height, called "leading". Instead we need to base ourselves on tightBoundingRect which is a rect only around the
                    // painted area of the label, not including the leading
                    topPadding: headMetrics.boundingRect.y - headMetrics.tightBoundingRect.y
                    level: root.compact ? 2 : 1
                    type: Kirigami.Heading.Type.Primary
                    text: root.application.name
                    elide: Text.ElideRight
                    maximumLineCount: 1
                    TextMetrics {
                        id: headMetrics
                        font: head.font
                        text: head.text
                    }
                }

                // Backend name label (shown if app is from a non-default backend and
                // we're not using the compact view, where there's no space for it)
                RowLayout {
                    Layout.alignment: Qt.AlignRight
                    visible: root.appIsFromNonDefaultBackend && !root.compact
                    spacing: Kirigami.Units.smallSpacing

                    Kirigami.Icon {
                        source: application.sourceIcon
                        implicitWidth: Kirigami.Units.iconSizes.smallMedium
                        implicitHeight: Kirigami.Units.iconSizes.smallMedium
                    }
                    Label {
                        text: application.backend.displayName
                        font: Kirigami.Theme.smallFont
                    }
                }
            }

            // Description/"Comment" label
            Label {
                id: description
                Layout.fillWidth: true
                Layout.preferredHeight: descriptionMetrics.height
                text: root.application.comment
                elide: Text.ElideRight
                maximumLineCount: 1
                textFormat: Text.PlainText

                // reserve space for description even if none is available
                TextMetrics {
                    id: descriptionMetrics
                    font: description.font
                    text: "Sample text"
                }
            }
            // Container for rating, size, and install button
            RowLayout {
                Layout.fillWidth: true
                spacing: Kirigami.Units.largeSpacing

                // Combined condition of both children items
                visible: root.showRating || (!root.compact && root.showSize) || !root.compact

                // Container for rating and size labels
                ColumnLayout {
                    Layout.fillWidth: true
                    // Include height of sizeInfo for full-sized view even when
                    // the actual sizeInfo layout isn't visible. This tightens up
                    // the layout and prevents the install button from appearing
                    // at a different position based on whether or not the
                    // sizeInfo text is visible, because the base layout is
                    // vertically centered rather than filling a distinct space.
                    Layout.preferredHeight: root.compact ? -1 : rating.implicitHeight + sizeInfo.implicitHeight
                    Layout.alignment: Qt.AlignBottom
                    spacing: 0

                    // Combined condition of both children items
                    visible: root.showRating || (!root.compact && root.showSize)

                    // Rating stars + label
                    RowLayout {
                        id: rating
                        Layout.fillWidth: true
                        Layout.alignment: Qt.AlignBottom
                        visible: root.showRating
                        opacity: 0.6
                        spacing: Kirigami.Units.largeSpacing

                        Rating {
                            Layout.alignment: Qt.AlignVCenter
                            value: root.application.rating ? root.application.rating.sortableRating : 0
                            starSize: root.compact ? description.font.pointSize : head.font.pointSize
                            precision: Rating.Precision.HalfStar
                            padding: 0
                        }
                        Label {
                            Layout.fillWidth: true
                            Layout.alignment: Qt.AlignVCenter
                            topPadding: (ratingLabelMetrics.boundingRect.y - ratingLabelMetrics.tightBoundingRect.y)/2
                            visible: root.application.rating || (root.application.backend.reviewsBackend && root.application.backend.reviewsBackend.isResourceSupported(root.application))
                            text: root.application.rating ? i18np("%1 rating", "%1 ratings", root.application.rating.ratingCount) : i18n("No ratings yet")
                            font: Kirigami.Theme.smallFont
                            elide: Text.ElideRight
                            TextMetrics {
                                id: ratingLabelMetrics
                                font: head.font
                                text: head.text
                            }
                        }
                    }

                    // Size label
                    Label {
                        id: sizeInfo
                        Layout.fillWidth: true
                        Layout.alignment: Qt.AlignBottom
                        visible: !root.compact && root.showSize
                        text: visible ? root.application.sizeDescription : ""
                        horizontalAlignment: Text.AlignRight
                        opacity: 0.6;
                        font: Kirigami.Theme.smallFont
                        elide: Text.ElideRight
                        maximumLineCount: 1
                    }
                }

                // Install button
                InstallApplicationButton {
                    id: installButton
                    Layout.alignment: Qt.AlignBottom | Qt.AlignRight
                    visible: !root.compact
                }
            }
        }
    }

    Accessible.name: head.text
    Accessible.onPressAction: trigger()
}
