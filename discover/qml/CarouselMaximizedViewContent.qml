/*
 *   SPDX-FileCopyrightText: 2023 ivan tkachenko <me@ratijas.tk>
 *
 *   SPDX-License-Identifier: LGPL-2.0-or-later
 */

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import QtQuick.Templates as T
import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.components as KirigamiComponents
import org.kde.discover

Item {
    id: root

    required property CarouselAbstractMaximizedView host

    readonly property real edgeMargin: Kirigami.Units.gridUnit

    readonly property real windowControlButtonsCollapsedMargin: host.toggleModeAvailable
        ? Math.round(edgeMargin / 2) : edgeMargin

    Kirigami.Theme.colorSet: Kirigami.Theme.Complementary
    Kirigami.Theme.inherit: false

    RowLayout {
        z: 2
        anchors.top: parent.top
        anchors.right: parent.right
        spacing: 0

        KirigamiComponents.FloatingButton {
            Kirigami.Theme.inherit: true

            text: {
                switch (root.host.mode) {
                case CarouselMaximizedViewController.Mode.FullScreen:
                    return i18nc("@action:button", "Switch to Overlay");
                case CarouselMaximizedViewController.Mode.Overlay:
                    return i18nc("@action:button", "Switch to Full Screen");
                }
                return "";
            }
            icon.name: {
                switch (root.host.mode) {
                case CarouselMaximizedViewController.Mode.FullScreen:
                    return "window-restore-symbolic";
                case CarouselMaximizedViewController.Mode.Overlay:
                    return "window-maximize-symbolic";
                }
                return "";
            }
            display: T.AbstractButton.IconOnly
            focusPolicy: Qt.NoFocus
            radius: Infinity

            margins: root.edgeMargin
            leftMargin: !mirrored ? margins : windowControlButtonsCollapsedMargin
            rightMargin: mirrored ? margins : windowControlButtonsCollapsedMargin

            visible: root.host.toggleModeAvailable

            QQC2.ToolTip.text: text
            QQC2.ToolTip.visible: (Kirigami.Settings.tabletMode ? pressed : hovered) && text !== ""
            QQC2.ToolTip.delay: Kirigami.Settings.tabletMode ? Qt.styleHints.mousePressAndHoldInterval : Kirigami.Units.toolTipDelay

            onClicked: root.host.toggleMode()
        }

        KirigamiComponents.FloatingButton {
            Kirigami.Theme.inherit: true

            text: i18nc("@action:button Close overlay/window/popup with carousel of screenshots", "Close")
            icon.name: "window-close-symbolic"
            display: T.AbstractButton.IconOnly
            focusPolicy: Qt.NoFocus
            radius: Infinity

            margins: root.edgeMargin
            leftMargin: mirrored ? margins : windowControlButtonsCollapsedMargin
            rightMargin: !mirrored ? margins : windowControlButtonsCollapsedMargin

            QQC2.ToolTip.text: text
            QQC2.ToolTip.visible: (Kirigami.Settings.tabletMode ? pressed : hovered) && text !== ""
            QQC2.ToolTip.delay: Kirigami.Settings.tabletMode ? Qt.styleHints.mousePressAndHoldInterval : Kirigami.Units.toolTipDelay

            onClicked: root.host.close(true);
        }
    }

    // Showcase some buttons for now
    CarouselNavigationButtons {
        Kirigami.Theme.inherit: true
        z: 1

        edgeMargin: root.edgeMargin
        atBeginning: false
        atEnd: false
    }
}
