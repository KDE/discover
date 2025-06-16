/*
 *   SPDX-FileCopyrightText: 2012 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *   SPDX-FileCopyrightText: 2023 ivan tkachenko <me@ratijas.tk>
 *
 *   SPDX-License-Identifier: LGPL-2.0-or-later
 */

import QtQuick
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami

QQC2.Control {
    id: root

    Kirigami.Theme.colorSet: Kirigami.Theme.Window
    Kirigami.Theme.inherit: false

    property alias text: theLabel.text
    property real progress: 1.0
    property bool selected: false

    readonly property bool inProgress: progress > 0

    padding: Kirigami.Units.smallSpacing * 1.5

    background: Item {
        visible: root.inProgress
        Rectangle {
            color: Qt.alpha(Kirigami.Theme.textColor, 0.1)
            border.color: root.selected ? Qt.alpha(Kirigami.Theme.highlightedTextColor, 0.2) : Qt.alpha(Kirigami.Theme.textColor, 0.2)
            border.width: 1
            anchors.fill: parent
            radius: root.padding
        }

        Rectangle {
            anchors {
                top: parent.top
                left: parent.left
                bottom: parent.bottom
            }
            color: Qt.alpha(Kirigami.Theme.highlightColor, 0.3)
            border.color: Kirigami.Theme.highlightColor
            border.width: 1
            radius: root.padding
            width: Math.round(parent.width * Math.max(0, Math.min(1, root.progress)))
            visible: width >= radius * 2
        }
    }

    contentItem: QQC2.Label {
        id: theLabel
        horizontalAlignment: Text.AlignHCenter
        color: root.selected ? Kirigami.Theme.highlightedTextColor : Kirigami.Theme.textColor
    }
}
