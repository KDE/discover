/*
 *   SPDX-FileCopyrightText: 2012 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *   SPDX-FileCopyrightText: 2018-2021 Nate Graham <nate@kde.org>
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

Kirigami.AbstractCard {
    id: delegateArea

    property alias application: installButton.application
    property bool compact: false
    property bool showRating: true
    property bool showSize: false

    readonly property bool appIsFromNonDefaultBackend: ResourcesModel.currentApplicationBackend !== application.backend && application.backend.hasApplications
    showClickFeedback: true

    function trigger() {
        const view = typeof delegateRecycler !== 'undefined' ? delegateRecycler.ListView.view : ListView.view
        if (view) {
            view.currentIndex = index
        }
        Navigation.openApplication(application)
    }
    highlighted: ListView.isCurrentItem || (typeof delegateRecycler !== 'undefined' && delegateRecycler.ListView.isCurrentItem)
    Keys.onReturnPressed: trigger()
    onClicked: trigger()

    contentItem: Item {
        implicitHeight: delegateArea.compact ? Kirigami.Units.gridUnit * 2 : Math.round(Kirigami.Units.gridUnit * 3.5)

        // App icon
        Kirigami.Icon {
            id: resourceIcon
            readonly property real contHeight: delegateArea.compact ? Kirigami.Units.iconSizes.large : Kirigami.Units.iconSizes.huge
            source: application.icon
            height: contHeight
            width: contHeight
            anchors {
                verticalCenter: parent.verticalCenter
                left: parent.left
                leftMargin: delegateArea.compact ? Kirigami.Units.largeSpacing : Kirigami.Units.largeSpacing * 2
            }
        }

        // Container for everything but the app icon
        ColumnLayout {
            anchors {
                verticalCenter: parent.verticalCenter
                right: parent.right
                left: resourceIcon.right
                leftMargin: Kirigami.Units.largeSpacing * 2
            }
            spacing: 0

            // Container for app name and backend name labels
            RowLayout {

                // App name label
                Kirigami.Heading {
                    id: head
                    Layout.fillWidth: true
                    level: delegateArea.compact ? 2 : 1
                    type: Kirigami.Heading.Type.Primary
                    text: delegateArea.application.name
                    elide: Text.ElideRight
                    maximumLineCount: 1
                }

                // Backend name label (shown if app is from a non-default backend and
                // we're not using the compact view, where there's no space for it)
                RowLayout {
                    Layout.alignment: Qt.AlignRight
                    visible: delegateArea.appIsFromNonDefaultBackend && !delegateArea.compact
                    spacing: 0

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
                text: delegateArea.application.comment
                elide: Text.ElideRight
                maximumLineCount: 1
                textFormat: Text.PlainText
            }

            // Container for rating, size, and install button
            RowLayout {
                Layout.fillWidth: true
                Layout.topMargin: delegateArea.compact ? Kirigami.Units.smallSpacing : Kirigami.Units.largeSpacing

                // Container for rating and size labels
                ColumnLayout {
                    Layout.fillWidth: true
                    // Include height of sizeInfo for full-sized view even when
                    // the actual sizeInfo layout isn't visible. This tightens up
                    // the layout and prevents the install button from appearing
                    // at a different position based on whether or not the
                    // sizeInfo text is visible, because the base layout is
                    // vertically centered rather than filling a distinct space.
                    Layout.preferredHeight: delegateArea.compact ? -1 : rating.implicitHeight + sizeInfo.implicitHeight
                    spacing: 0

                    // Rating stars + label
                    RowLayout {
                        id: rating
                        Layout.fillWidth: true
                        Layout.alignment: Qt.AlignTop
                        visible: showRating
                        opacity: 0.6
                        spacing: Kirigami.Units.largeSpacing

                        Rating {
                            rating: delegateArea.application.rating ? delegateArea.application.rating.sortableRating : 0
                            starSize: delegateArea.compact ? description.font.pointSize : head.font.pointSize
                        }
                        Label {
                            Layout.fillWidth: true
                            visible: delegateArea.application.rating || (delegateArea.application.backend.reviewsBackend && delegateArea.application.backend.reviewsBackend.isResourceSupported(delegateArea.application))
                            text: delegateArea.application.rating ? i18np("%1 rating", "%1 ratings", delegateArea.application.rating.ratingCount) : i18n("No ratings yet")
                            font: Kirigami.Theme.smallFont
                            elide: Text.ElideRight
                        }
                    }

                    // Size label
                    Label {
                        id: sizeInfo
                        Layout.fillWidth: true
                        visible: !delegateArea.compact && showSize
                        text: visible ? delegateArea.application.sizeDescription : ""
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
                    visible: !delegateArea.compact
                }
            }
        }
    }

    Accessible.name: head.text
    Accessible.onPressAction: trigger()
}
