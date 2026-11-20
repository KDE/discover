// SPDX-FileCopyrightText: 2026 Devin Lin <devin@kde.org>
// SPDX-License-Identifier: GPL-2.0-or-later

import QtQuick
import QtQuick.Layouts
import QtQuick.Templates as T

import org.kde.kirigami as Kirigami
import org.kde.bigscreen.ui as BigscreenUI
import org.kde.discover as Discover

BigscreenUI.ScrollablePage {
    id: root

    title: i18nc("@title", "Settings")

    BigscreenUI.BackHandler.onActivated: T.StackView.view.pop()

    ColumnLayout {
        width: root.availableWidth
        spacing: Kirigami.Units.smallSpacing

        BigscreenUI.KeyNavigationGroup.orientation: Qt.Vertical

        BigscreenUI.Label {
            Layout.fillWidth: true
            Layout.bottomMargin: Kirigami.Units.largeSpacing
            text: i18nc("@title", "Updates")
            type: BigscreenUI.Label.Heading
            color: Kirigami.Theme.disabledTextColor
        }

        BigscreenUI.ButtonDelegate {
            Layout.fillWidth: true
            focus: true
            text: i18nc("@action:button", "Check for updates")
            description: Discover.ResourcesModel.updatesCount > 0
                ? i18np("1 update available", "%1 updates available", Discover.ResourcesModel.updatesCount)
                : i18n("Look for new versions of your apps and system")
            icon.name: "view-refresh"
            enabled: Discover.ResourcesModel.updateAction.enabled
            onClicked: Discover.ResourcesModel.updateAction.trigger()
        }

        BigscreenUI.Label {
            Layout.fillWidth: true
            Layout.topMargin: Kirigami.Units.gridUnit * 2
            Layout.bottomMargin: Kirigami.Units.largeSpacing
            text: i18nc("@title", "Appearance")
            type: BigscreenUI.Label.Heading
            color: Kirigami.Theme.disabledTextColor
        }

        BigscreenUI.ColorSchemeDelegate {
            Layout.fillWidth: true
        }

        BigscreenUI.Label {
            Layout.fillWidth: true
            Layout.topMargin: Kirigami.Units.gridUnit * 2
            Layout.bottomMargin: Kirigami.Units.largeSpacing
            text: i18nc("@title", "About")
            type: BigscreenUI.Label.Heading
            color: Kirigami.Theme.disabledTextColor
        }

        BigscreenUI.AboutDelegate {
            Layout.fillWidth: true
        }
    }
}
