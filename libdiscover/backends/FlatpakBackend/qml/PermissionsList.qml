/*
 *   SPDX-FileCopyrightText: 2022 Suhaas Joshi <joshiesuhaas0@gmail.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.discover as Discover
import org.kde.kcmutils as KCMUtils
import org.kde.kirigami as Kirigami
import org.kde.kirigami.delegates as KD

ColumnLayout {
    id: root

    required property Discover.AbstractResource resource
    Discover.Activatable.active: list.model.rowCount() > 0
    spacing: 0

    Kirigami.Heading {
        Layout.fillWidth: true
        text: i18ndc("libdiscover", "Permission to access system resources and hardware devices", "Permissions")
        level: 2
        type: Kirigami.Heading.Type.Primary
        wrapMode: Text.Wrap
    }

    Repeater {
        id: list
        model: root.resource.permissionsModel()

        delegate: QQC2.ItemDelegate {
            id: delegate

            required property var model
            required property string brief
            required property string description

            Layout.fillWidth: true

            text: brief
            icon.name: model.icon

            // so that it gets neither hover nor pressed appearance when it's not interactive
            hoverEnabled: root.resource.isInstalled
            down: root.resource.isInstalled ? undefined : false

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
                if (root.resource.isInstalled) {
                    // TODO: Not only open KCM on the app's page, but also focus on relevant permission row
                    KCMUtils.KCMLauncher.openSystemSettings("kcm_flatpak", [root.resource.ref]);
                }
            }
        }
    }

    QQC2.Button {
        Layout.alignment: Qt.AlignHCenter
        Layout.maximumWidth: parent.width
        visible: root.resource.isInstalled
        text: i18nd("libdiscover", "Configure permissions…")
        icon.name: "configure"
        onClicked: {
            KCMUtils.KCMLauncher.openSystemSettings("kcm_flatpak", [root.resource.ref]);
        }
    }
}
