// SPDX-FileCopyrightText: 2026 Devin Lin <devin@kde.org>
// SPDX-License-Identifier: GPL-2.0-or-later

import QtQuick

import org.kde.kirigami as Kirigami
import org.kde.bigscreen.ui as BigscreenUI

BigscreenUI.ContentRail {
    id: root

    property real artworkWidth: Kirigami.Units.gridUnit * 14

    readonly property var currentApplication: currentItem?.application ?? null

    delegate: AppCard {
        artworkWidth: root.artworkWidth
    }
}
