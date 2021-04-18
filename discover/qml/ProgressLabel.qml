/*
 *   SPDX-FileCopyrightText: 2012 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: LGPL-2.0-or-later
 */

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import org.kde.discover.app 1.0
import org.kde.kirigami 2.14 as Kirigami

ProgressBar {
    id: root
    property alias text: theLabel.text
    padding: Kirigami.Units.smallSpacing * 1.5
    implicitWidth: Math.ceil(theLabel.implicitWidth) + leftPadding + rightPadding
    implicitHeight: Math.ceil(theLabel.implicitHeight) + topPadding + bottomPadding

    Label {
        id: theLabel
        parent: root
        anchors.fill: parent
        anchors.margins: padding
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
    }

    ToolTip.visible: hovered
    ToolTip.text: theLabel.text

    // Basic style workaround since it hardcodes background height and y values
    Binding {
        target: background
        when: background != null
        property: "height"
        value: root.height - root.topInset - root.bottomInset
    }
    Binding {
        target: background
        when: background != null
        property: "y"
        value: root.topInset
    }
}
