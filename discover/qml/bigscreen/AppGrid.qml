// SPDX-FileCopyrightText: 2026 Devin Lin <devin@kde.org>
// SPDX-License-Identifier: GPL-2.0-or-later

import QtQuick

import org.kde.kirigami as Kirigami
import org.kde.bigscreen.ui as BigscreenUI

BigscreenUI.MediaGrid {
    id: root

    // Whether the model is still looking for apps
    property bool busy: false

    // Whether to mark the installed apps
    property bool showInstalledState: true

    property alias placeholderText: placeholder.text
    property alias placeholderExplanation: placeholder.explanation
    property alias placeholderIconName: placeholder.iconName

    readonly property var currentApplication: currentItem?.application ?? null

    artworkAspectRatio: 16 / 9
    minimumCardWidth: Kirigami.Units.gridUnit * 14

    delegate: AppCard {
        showInstalledState: root.showInstalledState
        artworkWidth: root.cardWidth
        artworkHeight: root.artworkHeight
    }

    BigscreenUI.BusyIndicator {
        anchors.centerIn: parent
        visible: root.busy && root.count === 0
        running: visible
    }

    BigscreenUI.PlaceholderMessage {
        id: placeholder
        anchors.centerIn: parent
        width: Math.min(parent.width - root.leftMargin - root.rightMargin, implicitWidth)
        visible: !root.busy && root.count === 0 && text.length > 0
        iconName: "edit-none"
    }
}
