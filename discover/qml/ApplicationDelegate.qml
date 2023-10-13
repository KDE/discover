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
    highlighted: ListView.isCurrentItem
    Keys.onReturnPressed: trigger()
    onClicked: trigger()

    content: Item {
        implicitHeight: Math.max(columnLayout.implicitHeight, resourceIcon.height)

        // App icon
        Kirigami.Icon {
            id: resourceIcon
            readonly property real contHeight: root.compact ? Kirigami.Units.iconSizes.large : Kirigami.Units.iconSizes.huge
            source: application.icon
            animated: false
            height: contHeight
            width: contHeight
            anchors {
                verticalCenter: parent.verticalCenter
                left: parent.left
                leftMargin: root.compact ? Kirigami.Units.largeSpacing : Kirigami.Units.largeSpacing * 2
            }
        }

        // Container for everything but the app icon
        ColumnLayout {
            id: columnLayout

            anchors {
                verticalCenter: parent.verticalCenter
                right: parent.right
                left: resourceIcon.right
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
                    level: root.compact ? 2 : 1
                    type: Kirigami.Heading.Type.Primary
                    text: root.application.name
                    elide: Text.ElideRight
                    maximumLineCount: 1
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
                Layout.topMargin: root.compact ? Kirigami.Units.smallSpacing : Kirigami.Units.largeSpacing
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
                    spacing: 0

                    // Combined condition of both children items
                    visible: root.showRating || (!root.compact && root.showSize)

                    // Rating stars + label
                    RowLayout {
                        id: rating
                        Layout.fillWidth: true
                        Layout.alignment: Qt.AlignTop
                        visible: root.showRating
                        opacity: 0.6
                        spacing: Kirigami.Units.largeSpacing

                        Rating {
                            value: root.application.rating ? root.application.rating.sortableRating : 0
                            starSize: root.compact ? description.font.pointSize : head.font.pointSize
                            precision: Rating.Precision.HalfStar
                        }
                        Label {
                            Layout.fillWidth: true
                            visible: root.application.rating || (root.application.backend.reviewsBackend && root.application.backend.reviewsBackend.isResourceSupported(root.application))
                            text: root.application.rating ? i18np("%1 rating", "%1 ratings", root.application.rating.ratingCount) : i18n("No ratings yet")
                            font: Kirigami.Theme.smallFont
                            elide: Text.ElideRight
                        }
                    }

                    // Size label
                    Label {
                        id: sizeInfo
                        Layout.fillWidth: true
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
                    Layout.alignment: Qt.AlignVCenter | Qt.AlignRight
                    visible: !root.compact
                }
            }
        }
    }

    Accessible.name: head.text
    Accessible.onPressAction: trigger()
}
