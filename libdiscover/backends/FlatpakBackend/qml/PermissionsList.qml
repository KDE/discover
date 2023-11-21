/*
 *   SPDX-FileCopyrightText: 2022 Suhaas Joshi <joshiesuhaas0@gmail.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kcmutils as KCMUtils
import org.kde.kirigami as Kirigami
import org.kde.kirigami.delegates as KD

ColumnLayout {
    visible: list.model.rowCount() > 0
    spacing: 0

    Kirigami.Heading {
        Layout.fillWidth: true
        Layout.bottomMargin: Kirigami.Units.largeSpacing
        text: i18ndc("libdiscover", "%1 is the name of the application", "Permissions for %1", resource.name)
        level: 2
        type: Kirigami.Heading.Type.Primary
        wrapMode: Text.Wrap
    }

    Repeater {
        id: list
        model: resource.permissionsModel()

        delegate: QQC2.ItemDelegate {
            id: delegate

            required property var model
            required property string brief
            required property string description

            Layout.fillWidth: true

            text: brief
            icon.name: model.icon

            // so that it gets neither hover nor pressed appearance when it's not interactive
            hoverEnabled: resource.isInstalled
            down: resource.isInstalled ? undefined : false

            // ToolTip is intentionally omitted, as everything is wrapped and thus visible

            contentItem: KD.IconTitleSubtitle {
                icon: icon.fromControlsIcon(delegate.icon)
                title: delegate.text
                subtitle: delegate.description
                selected: delegate.highlighted || delegate.down
                font: delegate.font
                wrapMode: Text.Wrap
            }

            onClicked: {
                if (resource.isInstalled) {
                    // TODO: Not only open KCM on the app's page, but also focus on relevant permission row
                    KCMUtils.KCMLauncher.openSystemSettings("kcm_flatpak", [resource.ref]);
                }
            }
        }
    }

    QQC2.Button {
        Layout.alignment: Qt.AlignHCenter
        Layout.maximumWidth: parent.width
        Layout.topMargin: Kirigami.Units.largeSpacing
        visible: resource.isInstalled
        text: i18nd("libdiscover", "Configure permissions…")
        icon.name: "configure"
        onClicked: {
            KCMUtils.KCMLauncher.openSystemSettings("kcm_flatpak", [resource.ref]);
        }
    }
}
