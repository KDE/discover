/*
 *   SPDX-FileCopyrightText: 2022 Suhaas Joshi <joshiesuhaas0@gmail.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

import QtQuick 2.15
import QtQuick.Controls 2.15 as QQC2
import QtQuick.Layouts 1.12
import QtQml.Models 2.15
import org.kde.kirigami 2.14 as Kirigami
import org.kde.kquickcontrolsaddons 2.1 as KQuickControlsAddons

ColumnLayout {
    visible: list.model.rowCount() > 0

    Kirigami.Heading {
        Layout.fillWidth: true
        text: i18ndc("libdiscover", "%1 is the name of the application", "Permissions for %1", resource.name)
        level: 2
        type: Kirigami.Heading.Type.Primary
        wrapMode: Text.Wrap
    }

    Repeater {
        id: list
        model: resource.permissionsModel()

        delegate: Kirigami.BasicListItem {
            Layout.fillWidth: true
            text: model.brief
            subtitle: model.description
            icon: model.icon
            subtitleItem.wrapMode: Text.WordWrap

            // so that it gets neither hover nor pressed appearance when it's not interactive
            hoverEnabled: resource.isInstalled
            down: resource.isInstalled ? undefined : false

            onClicked: {
                if (resource.isInstalled) {
                    // TODO: Not only open KCM on the app's page, but also focus on relevant permission row
                    KQuickControlsAddons.KCMShell.openSystemSettings("kcm_flatpak", [resource.ref]);
                }
            }
        }
    }

    QQC2.Button {
        Layout.alignment: Qt.AlignHCenter
        Layout.maximumWidth: parent.width
        visible: resource.isInstalled
        text: i18nd("libdiscover", "Configure permissions…")
        icon.name: "configure"
        onClicked: {
            KQuickControlsAddons.KCMShell.openSystemSettings("kcm_flatpak", [resource.ref]);
        }
    }
}
