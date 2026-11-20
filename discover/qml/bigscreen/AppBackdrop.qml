// SPDX-FileCopyrightText: 2026 Devin Lin <devin@kde.org>
// SPDX-License-Identifier: GPL-2.0-or-later

import QtQuick
import QtQuick.Effects

import org.kde.kirigami as Kirigami
import org.kde.bigscreen.ui as BigscreenUI

// Soft light in the colors of an icon with an optional blurred image over it

Item {
    id: root

    // The app icon to take the color from
    property var icon

    // An image, such as a screenshot, to show over the light
    property alias source: image.source

    Kirigami.ImageColors {
        id: colors
        source: root.icon ?? ""
    }

    BigscreenUI.AmbientBackground {
        anchors.fill: parent
        tint: colors.dominant.a > 0 ? colors.dominant : Kirigami.Theme.highlightColor

        Behavior on tint { ColorAnimation { duration: Kirigami.Units.veryLongDuration } }
    }

    BigscreenUI.BackdropImage {
        id: image

        anchors.fill: parent
        artworkOpacity: 0.5

        layer.enabled: true
        layer.textureSize: Qt.size(width / 2, height / 2)
        layer.effect: MultiEffect {
            autoPaddingEnabled: false
            blurEnabled: true
            blur: 0.6
            blurMax: 32
        }
    }
}
