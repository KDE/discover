// SPDX-FileCopyrightText: 2026 Devin Lin <devin@kde.org>
// SPDX-License-Identifier: GPL-2.0-or-later

import QtQuick

import org.kde.kirigami as Kirigami
import org.kde.bigscreen.ui as BigscreenUI

Kirigami.ShadowedRectangle {
    id: root

    property var icon
    property real iconScale: 0.5

    readonly property color tint: colors.dominant.a > 0
        ? Kirigami.ColorUtils.tintWithAlpha(Kirigami.Theme.alternateBackgroundColor, colors.dominant, 0.35)
        : Kirigami.Theme.alternateBackgroundColor

    radius: BigscreenUI.Units.cardCornerRadius
    color: tint
    shadow.color: Qt.rgba(0, 0, 0, 0.5)

    Kirigami.ImageColors {
        id: colors
        source: root.icon ?? ""
    }

    Rectangle {
        anchors.fill: parent
        radius: root.radius
        gradient: Gradient {
            GradientStop { position: 0; color: Qt.rgba(1, 1, 1, 0.08) }
            GradientStop { position: 1; color: Qt.rgba(0, 0, 0, 0.15) }
        }
    }

    Kirigami.Icon {
        anchors.centerIn: parent
        width: Math.round(Math.min(parent.width, parent.height) * root.iconScale)
        height: width
        source: root.icon ?? "application-x-executable"
        fallback: "application-x-executable"
    }
}
