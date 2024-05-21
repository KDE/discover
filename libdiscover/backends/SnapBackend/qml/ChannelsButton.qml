/*
 *   SPDX-FileCopyrightText: 2018 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: LGPL-2.0-or-later
 */

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import org.kde.kirigami.delegates as KD
import org.kde.discover as Discover

QQC2.Button {
    id: root

    required property Discover.AbstractResource resource
    Discover.Activatable.active: resource.isInstalled /*&& view.count > 0*/

    text: i18nd("libdiscover", "Channel: %1", root.resource.channel)

    onClicked: overlay.open()
    visible: view.count > 1

    Kirigami.OverlaySheet {
        id: overlay
        width: parent.width / 2

        parent: root.QQC2.Overlay.overlay
        title: i18nd("libdiscover", "Channels for %1", root.resource.name)

        ListView {
            id: view

            model: root.resource.channels(root).channels
            delegate: QQC2.ItemDelegate {
                id: delegate
                width: parent.width

                required property var modelData

                readonly property bool current: root.resource.channel === modelData.name

                text: i18nd("libdiscover", "%1 - %2", modelData.name, modelData.version)

                contentItem: RowLayout {
                    spacing: Kirigami.Units.smallSpacing
                    KD.IconTitleSubtitle {
                        Layout.fillWidth: true
                                title: delegate.text
                                selected: delegate.highlighted
                                font: delegate.font
                            }

                    QQC2.Button {
                        text: i18nd("libdiscover", root.resource.isInstalled ? "Switch" : "Select")
                        enabled: !delegate.current
                        onClicked: {
                            root.resource.channel = delegate.modelData.name
                            overlay.close()
                        }
                    }
                }
            }
        }
    }
}
