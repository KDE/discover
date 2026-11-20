// SPDX-FileCopyrightText: 2026 Devin Lin <devin@kde.org>
// SPDX-License-Identifier: GPL-2.0-or-later

import QtQuick

import org.kde.kirigami as Kirigami
import org.kde.bigscreen.ui as BigscreenUI
import org.kde.discover as Discover

Item {
    id: root

    required property Discover.AbstractResource application

    // Hides the badge (such as while the app is being installed)
    property bool busy: false

    // Whether to mark installed apps or only those with updates
    property bool showInstalled: true

    readonly property bool upgradeable: application?.state === Discover.AbstractResource.Upgradeable
    readonly property bool installed: showInstalled && (application?.isInstalled ?? false)

    implicitWidth: upgradeable ? updateBadge.implicitWidth : installedMark.width
    implicitHeight: upgradeable ? updateBadge.implicitHeight : installedMark.height
    visible: !busy && (upgradeable || installed)

    BigscreenUI.Badge {
        id: updateBadge
        anchors.right: parent.right
        visible: root.upgradeable
        text: i18nc("@info:status the app has an update", "Update")
        color: Kirigami.Theme.highlightedTextColor
        background: Rectangle {
            radius: Kirigami.Units.cornerRadius
            color: Kirigami.Theme.highlightColor
        }
    }

    Rectangle {
        id: installedMark

        anchors.right: parent.right
        visible: root.installed && !root.upgradeable
        width: Kirigami.Units.iconSizes.medium
        height: width
        radius: width / 2
        color: Kirigami.Theme.highlightColor

        Accessible.name: i18nc("@info:status", "Installed")

        Kirigami.Icon {
            anchors.centerIn: parent
            width: Kirigami.Units.iconSizes.small
            height: width
            source: "checkmark"
            color: Kirigami.Theme.highlightedTextColor
            isMask: true
        }
    }
}
