// SPDX-FileCopyrightText: 2026 Devin Lin <devin@kde.org>
// SPDX-License-Identifier: GPL-2.0-or-later

import QtQuick
import QtQuick.Templates as T

import org.kde.kirigami as Kirigami
import org.kde.bigscreen.ui as BigscreenUI
import org.kde.discover as Discover

// Card for an app in a rail or grid

BigscreenUI.MediaCard {
    id: root

    required property Discover.AbstractResource application

    property bool showInstalledState: true

    property color tint: colors.dominant.a > 0
        ? Kirigami.ColorUtils.tintWithAlpha(Kirigami.Theme.alternateBackgroundColor, colors.dominant, 0.35)
        : Kirigami.Theme.alternateBackgroundColor

    artworkWidth: Kirigami.Units.gridUnit * 14
    artworkHeight: Math.round(artworkWidth * 9 / 16)
    artworkItem.color: tint

    text: application?.name ?? ""
    subtitle: application?.comment ?? ""
    progress: listener.isActive ? listener.progress / 100 : -1

    onClicked: T.ApplicationWindow.window.openApplication(application)

    Behavior on tint { ColorAnimation { duration: Kirigami.Units.longDuration } }

    Kirigami.ImageColors {
        id: colors
        source: root.application?.icon ?? ""
    }

    Discover.TransactionListener {
        id: listener
        resource: root.application
    }

    // Light background
    Rectangle {
        parent: root.artworkItem
        anchors.fill: parent
        anchors.margins: root.artworkItem.border.width
        radius: Math.max(0, root.artworkItem.radius - anchors.margins)
        gradient: Gradient {
            GradientStop {
                position: 0
                color: Qt.rgba(1, 1, 1, 0.08)
            }
            GradientStop {
                position: 1
                color: Qt.rgba(0, 0, 0, 0.15)
            }
        }
    }

    Kirigami.Icon {
        parent: root.artworkItem
        anchors.centerIn: parent
        width: Math.round(Math.min(parent.width, parent.height) * 0.5)
        height: width
        source: root.application?.icon ?? "application-x-executable"
        fallback: "application-x-executable"
    }

    StateBadge {
        parent: root.artworkItem
        anchors.top: parent.top
        anchors.right: parent.right
        anchors.margins: Kirigami.Units.largeSpacing
        application: root.application
        busy: listener.isActive
        showInstalled: root.showInstalledState
    }
}
