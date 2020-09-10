/*
 *   SPDX-FileCopyrightText: 2018 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: LGPL-2.0-or-later
 */

import QtQuick 2.1
import QtQuick.Controls 2.1
import org.kde.kirigami 2.1 as Kirigami

Button
{
    id: root
    text: i18n("Configure permissions...")

    onClicked: overlay.open()
    visible: resource.isInstalled && view.count > 0

    DiscoverPopup {
        id: overlay

        ListView {
            id: view
            anchors.fill: parent
            header: Kirigami.ItemViewHeader {
                title: i18n ("Permissions for %1", resource.name)
            }
            model: resource.plugs(root)
            delegate: CheckDelegate {
                id: delegate
                width: parent.width
                text: model.display
                checked: model.checked
                onClicked: {
                    model.checked = delegate.checked
                }
            }
        }
    }
}
