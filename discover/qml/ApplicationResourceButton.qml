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
    required property string title
    required property string subtitle
    required property string website

    spacing: Kirigami.Units.smallSpacing

    Kirigami.Icon {
        id: icon
        Layout.preferredWidth: Kirigami.Units.iconSizes.medium
        Layout.preferredHeight: Kirigami.Units.iconSizes.medium
        Layout.alignment: Qt.AlignVCenter
        source: root.icon
    }

    ColumnLayout {
        spacing: 0

        // Title
        Kirigami.Heading {
            Layout.fillWidth: true
            level: 4
            text: root.title
            maximumLineCount: 2
            elide: Text.ElideRight
            wrapMode: Text.Wrap
            horizontalAlignment: Text.AlignLeft
            verticalAlignment: Text.AlignBottom
        }

        // Subtitle
        Kirigami.UrlButton {
            Layout.fillWidth: true
            visible: root.subtitle
            text: root.subtitle
            url: root.website
            wrapMode: Text.Wrap
            horizontalAlignment: Text.AlignLeft
            verticalAlignment: Text.AlignTop
        }
    }
}
