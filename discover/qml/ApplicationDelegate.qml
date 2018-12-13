/*
 *   Copyright (C) 2012 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library/Lesser General Public License
 *   version 2, or (at your option) any later version, as published by the
 *   Free Software Foundation
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details
 *
 *   You should have received a copy of the GNU Library/Lesser General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

import QtQuick 2.1
import QtQuick.Controls 2.1
import QtQuick.Layouts 1.1
import QtQuick.Window 2.1
import "navigation.js" as Navigation
import org.kde.kirigami 2.4 as Kirigami

Kirigami.AbstractCard
{
    id: delegateArea
    property alias application: installButton.application
    property bool compact: false
    property bool showRating: true
    showClickFeedback: true
    property var view: null

    function trigger() {
        if (view)
            view.currentIndex = index
        Navigation.openApplication(application)
    }
    highlighted: ListView.isCurrentItem
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

            Kirigami.Heading {
                id: head
                level: delegateArea.compact ? 3 : 2
                Layout.fillWidth: true
                elide: Text.ElideRight
                text: delegateArea.application.name
                maximumLineCount: 1
            }

            InstallApplicationButton {
                id: installButton
                Layout.alignment: Qt.AlignVCenter | Qt.AlignRight
                Layout.rowSpan: delegateArea.compact ? 3 : 1
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
                    visible: delegateArea.application.rating || delegateArea.application.backend.reviewsBackend.isResourceSupported(delegateArea.application)
                    opacity: 0.5
                    elide: Text.ElideRight
                }
            }

            Label {
                Layout.columnSpan: delegateArea.compact ? 1 : 2
                id: summary
                Layout.fillWidth: true

                bottomPadding: Kirigami.Units.smallSpacing
                elide: Text.ElideRight
                text: delegateArea.application.comment
                maximumLineCount: 1
                textFormat: Text.PlainText
            }
        }
    }
}
