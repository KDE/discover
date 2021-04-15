/*
 *   SPDX-FileCopyrightText: 2012 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: LGPL-2.0-or-later
 */

import QtQuick 2.15
import QtQuick.Controls 2.1
import QtQuick.Layouts 1.1
import QtQuick.Window 2.1
import "navigation.js" as Navigation
import org.kde.kirigami 2.14 as Kirigami

Kirigami.AbstractCard
{
    id: delegateArea
    property alias application: installButton.application
    property bool compact: false
    property bool showRating: true

    readonly property bool appIsFromNonDefaultBackend: ResourcesModel.currentApplicationBackend !== application.backend && application.backend.hasApplications
    showClickFeedback: true

    function trigger() {
        if (delegateRecycler.ListView.view)
            delegateRecycler.ListView.view.currentIndex = index
        Navigation.openApplication(application)
    }
    highlighted: delegateRecycler && delegateRecycler.ListView.isCurrentItem
    Keys.onReturnPressed: trigger()
    onClicked: trigger()

    contentItem: Item {
        implicitHeight: delegateArea.compact ? Kirigami.Units.gridUnit * 2 : Kirigami.Units.gridUnit * 4

        Kirigami.Icon {
            id: resourceIcon
            source: application.icon
            readonly property real contHeight: delegateArea.compact ? Kirigami.Units.gridUnit * 3 : Kirigami.Units.gridUnit * 5
            height: contHeight
            width: contHeight
            anchors {
                verticalCenter: parent.verticalCenter
                left: parent.left
            }
        }

        GridLayout {
            columnSpacing: delegateArea.compact ? 0 : 5
            rowSpacing: delegateArea.compact ? 0 : 5
            anchors {
                verticalCenter: parent.verticalCenter
                right: parent.right
                left: resourceIcon.right
                leftMargin: Kirigami.Units.largeSpacing
            }
            columns: 2
            rows: delegateArea.compact ? 4 : 3

            RowLayout {
                Layout.fillWidth: true
                readonly property bool bigTitle: (head.implicitWidth + installButton.width) > parent.width

                Kirigami.Heading {
                    id: head
                    level: delegateArea.compact ? 3 : 2
                    Layout.fillWidth: !category.visible || parent.bigTitle
                    elide: Text.ElideRight
                    text: delegateArea.application.name
                    maximumLineCount: 1
                }

                Kirigami.Heading {
                    id: category
                    level: 5
                    Layout.fillWidth: true
                    elide: Text.ElideRight
                    text: i18nc("Part of a string like this: '<app name> - <category>'", "- %1", delegateArea.application.categoryDisplay)
                    maximumLineCount: 1
                    opacity: 0.6
                    visible: delegateArea.application.categoryDisplay && delegateArea.application.categoryDisplay !== page.title && !parent.bigTitle
                }
            }

            InstallApplicationButton {
                id: installButton
                Layout.alignment: Qt.AlignVCenter | Qt.AlignRight
                Layout.rowSpan: delegateArea.compact ? 3 : 1
                compact: delegateArea.compact
                backendName: delegateArea.appIsFromNonDefaultBackend ? application.backend.displayName : ""

                // TODO: Show the backend icon inside the button for Flatpak
                // and Snap backend items in widescreen mode. Currently blocked
                // by https://bugs.kde.org/show_bug.cgi?id=433433

            }

            RowLayout {
                visible: showRating
                spacing: Kirigami.Units.largeSpacing
                Layout.fillWidth: true
                Rating {
                    rating: delegateArea.application.rating ? delegateArea.application.rating.sortableRating : 0
                    starSize: delegateArea.compact ? summary.font.pointSize : head.font.pointSize
                }
                Label {
                    Layout.fillWidth: true
                    text: delegateArea.application.rating ? i18np("%1 rating", "%1 ratings", delegateArea.application.rating.ratingCount) : i18n("No ratings yet")
                    visible: delegateArea.application.rating || (delegateArea.application.backend.reviewsBackend && delegateArea.application.backend.reviewsBackend.isResourceSupported(delegateArea.application))
                    opacity: 0.5
                    elide: Text.ElideRight
                }
            }

            Label {
                id: summary
                Layout.columnSpan: delegateArea.compact ? 1 : 2
                Layout.fillWidth: true

                rightPadding: soup.visible ? soup.width + soup.anchors.rightMargin : 0
                elide: Text.ElideRight
                text: delegateArea.application.comment
                maximumLineCount: 1
                textFormat: Text.PlainText

                // TODO: remove this once the backend icon is inside the install
                // button (blocked by https://bugs.kde.org/show_bug.cgi?id=433433)
                Kirigami.Icon {
                    id: soup
                    source: application.sourceIcon
                    height: Kirigami.Units.gridUnit
                    width: Kirigami.Units.gridUnit
                    smooth: true
                    visible: !delegateArea.compact && delegateArea.appIsFromNonDefaultBackend

                    HoverHandler {
                        id: sourceIconHover
                    }

                    ToolTip.text: application.backend.displayName
                    ToolTip.visible: sourceIconHover.hovered && soup.visible
                    ToolTip.delay: Kirigami.Units.toolTipDelay

                    anchors {
                        bottom: parent.bottom
                        right: parent.right
                        rightMargin: Kirigami.Units.smallSpacing
                    }
                }
            }
        }
    }
}
