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
        const view = typeof delegateRecycler !== 'undefined' ? delegateRecycler.ListView.view : ListView.view
        if (view)
            view.currentIndex = index
        Navigation.openApplication(application)
    }
    highlighted: ListView.isCurrentItem || (typeof delegateRecycler !== 'undefined' && delegateRecycler.ListView.isCurrentItem)
    Keys.onReturnPressed: trigger()
    onClicked: trigger()

    contentItem: Item {
        implicitHeight: delegateArea.compact ? Kirigami.Units.gridUnit * 2 : Kirigami.Units.gridUnit * 4

        Kirigami.Icon {
            id: resourceIcon
            readonly property real contHeight: delegateArea.compact ? Kirigami.Units.gridUnit * 3 : Kirigami.Units.gridUnit * 5
            source: application.icon
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
                readonly property bool bigTitle: (head.implicitWidth + installButton.width) > parent.width
                Layout.fillWidth: true

                Kirigami.Heading {
                    id: head
                    Layout.fillWidth: !category.visible || parent.bigTitle
                    level: delegateArea.compact ? 3 : 2
                    text: delegateArea.application.name
                    elide: Text.ElideRight
                    maximumLineCount: 1
                }

                Kirigami.Heading {
                    id: category
                    Layout.fillWidth: true
                    visible: delegateArea.application.categoryDisplay && delegateArea.application.categoryDisplay !== page.title && !parent.bigTitle
                    level: 5
                    opacity: 0.6
                    text: i18nc("Part of a string like this: '<app name> - <category>'", "- %1", delegateArea.application.categoryDisplay)
                    elide: Text.ElideRight
                    maximumLineCount: 1
                }
            }

            InstallApplicationButton {
                id: installButton
                Layout.alignment: Qt.AlignVCenter | Qt.AlignRight
                Layout.rowSpan: delegateArea.compact ? 3 : 1
                compact: delegateArea.compact
                appIsFromNonDefaultBackend: delegateArea.appIsFromNonDefaultBackend
                backendName: delegateArea.appIsFromNonDefaultBackend ? application.backend.displayName : ""
            }

            RowLayout {
                Layout.fillWidth: true
                visible: showRating
                spacing: Kirigami.Units.largeSpacing

                Rating {
                    rating: delegateArea.application.rating ? delegateArea.application.rating.sortableRating : 0
                    starSize: delegateArea.compact ? summary.font.pointSize : head.font.pointSize
                }
                Label {
                    Layout.fillWidth: true
                    visible: delegateArea.application.rating || (delegateArea.application.backend.reviewsBackend && delegateArea.application.backend.reviewsBackend.isResourceSupported(delegateArea.application))
                    opacity: 0.5
                    text: delegateArea.application.rating ? i18np("%1 rating", "%1 ratings", delegateArea.application.rating.ratingCount) : i18n("No ratings yet")
                    elide: Text.ElideRight
                }
            }

            Label {
                id: summary
                Layout.columnSpan: delegateArea.compact ? 1 : 2
                Layout.fillWidth: true
                text: delegateArea.application.comment
                elide: Text.ElideRight
                maximumLineCount: 1
                textFormat: Text.PlainText
            }
        }
    }
}
