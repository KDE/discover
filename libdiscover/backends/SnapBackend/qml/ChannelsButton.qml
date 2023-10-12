/*
 *   SPDX-FileCopyrightText: 2018 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: LGPL-2.0-or-later
 */

import QtQuick 2.1
import QtQuick.Layouts 1.1
import QtQuick.Controls 2.1
import org.kde.kirigami 2.14 as Kirigami
import org.kde.kirigami.delegates as KD

Button {
    id: root

    text: i18nd("libdiscover", "Channels…")

    onClicked: overlay.open()
    visible: resource.isInstalled /*&& view.count > 0*/

    Kirigami.OverlaySheet {
        id: overlay

        parent: applicationWindow().overlay
        title: i18nd("libdiscover", "%1 channels", resource.name)

        ListView {
            id: view

            model: resource.channels(root).channels
            delegate: ItemDelegate {
                id: delegate
                readonly property bool current: resource.channel === modelData.name
                text: i18nd("libdiscover", "%1 - %2", modelData.name, modelData.version)

                contentItem: RowLayout {
                    spacing: Kirigami.Units.smallSpacing
                    KD.IconTitleSubtitle {
                        Layout.fillWidth: true
                        icon: icon.fromControlsIcon(delegate.icon)
                        title: delegate.text
                        selected: delegate.highlighted
                        font: delegate.font
                    }

                    Button {
                        text: i18nd("libdiscover", "Switch")
                        enabled: !delegate.current
                        onClicked: resource.channel = modelData.name
                    }
                }
            }
        }
    }
}
