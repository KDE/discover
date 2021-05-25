/*
 *   SPDX-FileCopyrightText: 2018 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: LGPL-2.0-or-later
 */

import QtQuick 2.1
import QtQuick.Layouts 1.1
import QtQuick.Controls 2.1
import org.kde.kirigami 2.14 as Kirigami

Button
{
    id: root
    text: i18n("Channels…")

    onClicked: overlay.open()
    visible: resource.isInstalled /*&& view.count > 0*/

    DiscoverPopup {
        id: overlay

        ListView {
            id: view
            anchors.fill: parent
            header: Kirigami.ItemViewHeader {
                title: i18n ("%1 channels", resource.name)
            }
            model: resource.channels(root).channels
            delegate: RowLayout {
                width: view.width
                readonly property bool current: resource.channel === modelData.name
                Label {
                    Layout.fillWidth: true
                    text: i18n("%1 - %2", modelData.name, modelData.version)
                }
                Button {
                    text: i18n("Switch")
                    enabled: !parent.current
                    onClicked: resource.channel = modelData.name
                }
            }
        }
    }
}
