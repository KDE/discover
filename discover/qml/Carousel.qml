// SPDX-FileCopyrightText: 2021 Carl Schwan <carlschwan@kde.org>
// SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
//
import QtQuick 2.15
import QtGraphicalEffects 1.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import org.kde.kirigami 2.14 as Kirigami

PathView {
    id: pathView
    readonly property bool itemIsWide: pathView.width / 3 > Kirigami.Units.gridUnit * 14
    /// Item width on small scren: only show one item fully and partially the left and right item
    // (Kirigami.Units.gridUnit * 2 for each item)
    readonly property int itemWidthSmall: width - smallExternalMargins * 2

    /// Item width on large screen: e.g. 3 item always displayed
    readonly property int itemWidthLarge: pathView.width / 3

    readonly property int smallExternalMargins: width > Kirigami.Units.gridUnit * 25 ? Kirigami.Units.gridUnit * 4 : Kirigami.Units.gridUnit * 2
    pathItemCount: itemIsWide ? 5 : 3
    preferredHighlightBegin: 0.5
    preferredHighlightEnd: 0.5
    highlightRangeMode: PathView.StrictlyEnforceRange

    property var timer: Timer {
        running: true
        interval: 5000
        repeat: true
        onTriggered: pathView.incrementCurrentIndex()
    }

    delegate: ItemDelegate {
        id: colorfulRectangle
        width: (pathView.itemIsWide ? pathView.itemWidthLarge : pathView.itemWidthSmall) - Kirigami.Units.gridUnit * 2
        x: Kirigami.Units.gridUnit
        height: PathView.view.height
        onClicked: Navigation.openApplication(applicationObject)
        background: Rectangle {
            radius: Kirigami.Units.largeSpacing
            gradient: Gradient {
                GradientStop { position: 0.0; color: model.gradientStart}
                GradientStop { position: 1.0; color: model.gradientEnd}
            }
        }
        ColumnLayout {
            anchors {
                centerIn: parent
            }
            Kirigami.Icon {
                id: resourceIcon
                source: model.applicationObject.icon
                Layout.preferredHeight: Kirigami.Units.gridUnit * 3
                Layout.preferredWidth: Kirigami.Units.gridUnit * 3
                Layout.alignment: Qt.AlignHCenter
            }
            Label {
                color: model.color
                text: model.applicationObject.name
                font.pointSize: 24
                Layout.alignment: Qt.AlignHCenter
                horizontalAlignment: Text.AlignHCenter
            }


            Kirigami.Heading {
                color: model.color
                level: 2
                wrapMode: Text.WordWrap
                Layout.alignment: Qt.AlignHCenter
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignTop
                Layout.maximumWidth: colorfulRectangle.width - Kirigami.Units.largeSpacing * 2
                text: model.applicationObject.comment
                maximumLineCount: 2
                elide: Text.ElideRight
                Layout.preferredHeight: lineCount === 1 ? contentHeight * 2 + topPadding + bottomPadding : implicitHeight
            }
        }
    }
    path: Path {
        startX: pathView.itemIsWide ? (-pathView.width / 3) : (-pathView.itemWidthSmall + pathView.smallExternalMargins)
        startY: pathView.height/2
        PathLine {
            x: pathView.width / 2
            y: pathView.height / 2
        }
        PathLine {
            x: pathView.itemIsWide ? pathView.width * 4 / 3 : (pathView.width + pathView.itemWidthSmall - pathView.smallExternalMargins)
            y: pathView.height / 2
        }
    }
}

