/*
 *   SPDX-FileCopyrightText: 2012 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: LGPL-2.0-or-later
 */

import QtQuick 2.1
import QtQuick.Layouts 1.1
import QtQuick.Controls 2.1
import org.kde.discover 2.0
import org.kde.kirigami 2.14 as Kirigami

RowLayout {
    id: item
    visible: model.shouldShow
    property bool compact: false
    property bool separator: true
    signal markUseful(bool useful)

    // Spacers to indent nested comments/replies
    Repeater {
        model: depth
        delegate: Item {
            Layout.fillHeight: true
            Layout.minimumWidth: Kirigami.Units.largeSpacing
            Layout.maximumWidth: Kirigami.Units.largeSpacing
        }
    }

    Rectangle {
        id: reviewBox
        color: "transparent"
        border.color: Qt.tint(Kirigami.Theme.textColor, Qt.rgba(Kirigami.Theme.backgroundColor.r, Kirigami.Theme.backgroundColor.g, Kirigami.Theme.backgroundColor.b, 0.85))
        border.width: 1
        radius: Kirigami.Units.largeSpacing

        Layout.fillWidth: true
        implicitHeight: reviewLayout.implicitHeight + (Kirigami.Units.largeSpacing * 2)

        ColumnLayout {
            id: reviewLayout
            anchors.fill: parent
            spacing: 0

            // Header with stars and date of review
            RowLayout {
                Layout.fillWidth: true
                Layout.leftMargin: Kirigami.Units.largeSpacing
                Layout.rightMargin: Kirigami.Units.largeSpacing
                Layout.topMargin: Kirigami.Units.largeSpacing
                Layout.bottomMargin: Kirigami.Units.smallSpacing

                Rating {
                    id: rating
                    rating: model.rating
                    starSize: content.font.pointSize
                }

                Label {
                    text: packageVersion ? i18n("Version: %1", packageVersion) : i18n("Version: unknown")
                    elide: Text.ElideRight
                    opacity: 0.6
                }

                Label {
                    Layout.fillWidth: true
                    horizontalAlignment: Text.AlignRight
                    text: date.toLocaleDateString(Qt.locale(), Locale.LongFormat)
                    elide: Text.ElideRight
                    opacity: 0.6
                }
            }

            // Review title and author
            Label {
                id: content
                Layout.fillWidth: true
                Layout.leftMargin: Kirigami.Units.largeSpacing
                Layout.rightMargin: Kirigami.Units.largeSpacing
                Layout.bottomMargin: Kirigami.Units.smallSpacing

                elide: Text.ElideRight
                readonly property string author: reviewer ? reviewer : i18n("unknown reviewer")
                text: summary ? i18n("<b>%1</b> by %2", summary, author) : i18n("Comment by %1", author)
            }

            // Review text
            Label {
                Layout.fillWidth: true
                Layout.leftMargin: Kirigami.Units.largeSpacing
                Layout.rightMargin: Kirigami.Units.largeSpacing
                Layout.bottomMargin: Kirigami.Units.smallSpacing

                text: display
                wrapMode: Text.Wrap
            }

            // Gray bar at the bottom + its content layout
            Rectangle {
                color: Kirigami.Theme.backgroundColor
                Layout.fillWidth: true
                // To make the background color rect touch the outline of the box
                Layout.leftMargin: reviewBox.border.width
                Layout.rightMargin: reviewBox.border.width
                implicitHeight: rateTheReviewLayout.implicitHeight
                radius: Kirigami.Units.largeSpacing
                visible: !item.compact

                // This fills in the rounded top corners, because you can't have a
                // rectangle with only a few corners rounded
                Rectangle {
                    color: Kirigami.Theme.backgroundColor
                    border.color: Qt.tint(Kirigami.Theme.textColor, Qt.rgba(Kirigami.Theme.backgroundColor.r, Kirigami.Theme.backgroundColor.g, Kirigami.Theme.backgroundColor.b, 0.85))
                    anchors.left: parent.left
                    anchors.leftMargin: -1
                    anchors.right: parent.right
                    anchors.rightMargin: -1
                    anchors.top: parent.top
                    anchors.topMargin: -1
                    z: -1
                    height: reviewBox.radius
                }

                RowLayout {
                    id: rateTheReviewLayout
                    anchors.fill: parent
                    anchors.leftMargin: Kirigami.Units.largeSpacing
                    anchors.rightMargin: Kirigami.Units.smallSpacing

                    Label {
                        visible: usefulnessTotal !== 0
                        text: i18n("Votes: %1 out of %2", usefulnessFavorable, usefulnessTotal)
                    }

                    Label {
                        Layout.fillWidth: true
                        horizontalAlignment: Text.AlignRight
                        text: i18n("Was this review useful?")
                        elide: Text.ElideLeft
                    }

                    // Useful/Not Useful buttons
                    Button {
                        id: yesButton
                        Layout.maximumWidth: Kirigami.Units.gridUnit * 3
                        Layout.topMargin: Kirigami.Units.smallSpacing
                        Layout.bottomMargin: Kirigami.Units.smallSpacing
                        Layout.alignment: Qt.AlignVCenter

                        text: i18nc("Keep this string as short as humanly possible", "Yes")

                        checkable: true
                        checked: usefulChoice === ReviewsModel.Yes
                        onClicked: {
                            noButton.checked = false
                            item.markUseful(true)
                        }
                    }
                    Button {
                        id: noButton
                        Layout.maximumWidth: Kirigami.Units.gridUnit * 3
                        Layout.topMargin: Kirigami.Units.smallSpacing
                        Layout.bottomMargin: Kirigami.Units.smallSpacing
                        Layout.alignment: Qt.AlignVCenter

                        text: i18nc("Keep this string as short as humanly possible", "No")

                        checkable: true
                        checked: usefulChoice === ReviewsModel.No
                        onClicked: {
                            yesButton.checked = false
                            item.markUseful(false)
                        }
                    }
                }
            }
        }
    }
}
