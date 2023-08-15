/*
 *   SPDX-FileCopyrightText: 2023 ivan tkachenko <me@ratijas.tk>
 *
 *   SPDX-License-Identifier: LGPL-2.0-or-later
 */

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Templates as T
import Qt5Compat.GraphicalEffects as GE

import org.kde.kirigami 2 as Kirigami

T.RoundButton {
    id: controlRoot

    required property ListView view

    property int role: Qt.AlignLeading

    readonly property int effectiveRole: mirrored ? (role === Qt.AlignLeading ? Qt.AlignTrailing : Qt.AlignLeading) : role

    property real extraEdgePadding: Kirigami.Units.gridUnit

    Kirigami.Theme.colorSet: Kirigami.Theme.Button
    Kirigami.Theme.inherit: flat && !down && !checked

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding,
                             implicitIndicatorHeight + topPadding + bottomPadding)

    anchors {
        left: role === Qt.AlignLeading ? parent?.left : undefined
        right: role === Qt.AlignTrailing ? parent?.right : undefined
    }

    height: parent?.height ?? undefined

    leftInset: effectiveRole === Qt.AlignLeading ? Kirigami.Units.largeSpacing + extraEdgePadding : 0
    rightInset: effectiveRole === Qt.AlignTrailing ? Kirigami.Units.largeSpacing + extraEdgePadding : 0

    padding: Kirigami.Units.largeSpacing + Kirigami.Units.smallSpacing
    leftPadding: horizontalPadding + leftInset
    rightPadding: horizontalPadding + rightInset

    icon.name: effectiveRole === Qt.AlignLeading
        ? "arrow-left" : "arrow-right"
    readonly property int iconSize: Kirigami.Units.iconSizes.roundedIconSize(Math.min(availableWidth, availableHeight))
    icon.width: iconSize
    icon.height: iconSize
    icon.color: Qt.tint(controlRoot.palette.buttonText, Qt.alpha(Kirigami.Theme.backgroundColor, 0.5))

    text: effectiveRole === Qt.AlignLeading
        ? i18n("Previous Screenshot") : i18n("Next Screenshot")

    focusPolicy: Qt.NoFocus
    display: QQC2.AbstractButton.IconOnly

    enabled: effectiveRole === Qt.AlignLeading
        ? view.currentIndex !== 0
        : view.currentIndex !== view.count - 1

    opacity: enabled ? 1 : 0

    Behavior on opacity {
        NumberAnimation {
            duration: Kirigami.Units.longDuration
            easing.type: Easing.OutCubic
        }
    }

    transform: Translate {
        x: controlRoot.enabled ? 0
            : Kirigami.Units.gridUnit * 2 * (controlRoot.effectiveRole === Qt.AlignLeading ? -1 : 1)

        Behavior on x {
            NumberAnimation {
                duration: Kirigami.Units.longDuration
                easing.type: Easing.OutCubic
            }
        }
    }

    contentItem: Item {
        implicitWidth: Kirigami.Units.iconSizes.smallMedium
        implicitHeight: Kirigami.Units.iconSizes.smallMedium

        Kirigami.Icon {
            anchors.centerIn: parent
            width: controlRoot.icon.width
            height: controlRoot.icon.height
            color: controlRoot.icon.color // defaults to Qt::transparent
            source: controlRoot.icon.name !== "" ? controlRoot.icon.name : controlRoot.icon.source
        }
    }

    background: Item {
        GE.DropShadow {
            anchors.fill: backgroundImpl
            horizontalOffset: 0
            verticalOffset: 5
            radius: 8.0
            samples: 17
            color: "#40000000"
            source: backgroundImpl
        }

        Rectangle {
            id: backgroundImpl

            anchors.centerIn: parent
            width: Math.min(parent.width, parent.height)
            height: width
            radius: width

            color: if (controlRoot.down) {
                Qt.tint(Kirigami.Theme.backgroundColor, Qt.alpha(Kirigami.Theme.textColor, 0.1))
            } else {
                return Kirigami.Theme.backgroundColor;
            }
            border.width: 1
            border.color: Qt.alpha(controlRoot.icon.color, 0.3)

            Behavior on color {
                ColorAnimation {
                    duration: Kirigami.Units.veryShortDuration
                }
            }

            // Put tooltip here, so it doesn't show up all the way on the top of the gallery
            QQC2.ToolTip.text: controlRoot.text
            QQC2.ToolTip.visible: controlRoot.text !== "" && controlRoot.enabled && (Kirigami.Settings.isMobile ? controlRoot.pressed : controlRoot.hovered)
            QQC2.ToolTip.delay: Kirigami.Units.toolTipDelay
        }
    }

    onClicked: {
        // Don't want it to get in the way of vieweing screenshots.
        backgroundImpl.QQC2.ToolTip.hide();
    }
}
