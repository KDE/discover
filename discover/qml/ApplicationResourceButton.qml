/*
 *   SPDX-FileCopyrightText: 2022 Nate Graham <nate@kde.org>
 *
 *   SPDX-License-Identifier: LGPL-2.0-or-later
 */

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2

import org.kde.kirigami as Kirigami

RowLayout {
    id: root

    required property string icon
    required property string website
    required property string linkText

    spacing: Kirigami.Units.smallSpacing

    Kirigami.Icon {
        id: icon
        Layout.preferredWidth: Kirigami.Units.iconSizes.sizeForLabels
        Layout.preferredHeight: Kirigami.Units.iconSizes.sizeForLabels
        Layout.alignment: Qt.AlignVCenter
        source: root.icon
    }

    Kirigami.UrlButton {
        Layout.fillWidth: true
        visible: root.linkText
        text: root.linkText
        url: root.website
        wrapMode: Text.Wrap
        horizontalAlignment: Text.AlignLeft
        verticalAlignment: Text.AlignTop
    }
}
