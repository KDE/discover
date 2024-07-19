/*
 *   SPDX-FileCopyrightText: 2018 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: LGPL-2.0-or-later
 */

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import org.kde.discover as Discover

QQC2.Button {
    id: root

    required property Discover.AbstractResource resource
    Discover.Activatable.active: resource.isInstalled && view.count > 0

    text: i18nd("libdiscover", "Configure permissions…")

    onClicked: overlay.open()

    Kirigami.OverlaySheet {
        id: overlay

        parent: root.QQC2.Overlay.overlay
        title: i18nd("libdiscover", "Permissions for %1", root.resource.name)

        property Discover.InlineMessage errorMessage

        ListView {
            id: view
            model: root.resource.plugs(root)
            Connections {
                target: view.model
                function onError(message) {
                    overlay.errorMessage = message
                }
            }
            header: DiscoverInlineMessage {
                inlineMessage: overlay.errorMessage
            }
            delegate: QQC2.CheckDelegate {
                id: delegate

                required property var model

                width: view.width
                text: model.display
                checked: model.checked
                onToggled: {
                    model.checked = checked
                }
            }
        }
    }
}
