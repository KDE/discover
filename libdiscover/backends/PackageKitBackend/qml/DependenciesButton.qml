/*
 *   SPDX-FileCopyrightText: 2018 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: LGPL-2.0-or-later
 */

import QtQuick 2.8
import QtQuick.Controls 2.1
import org.kde.kirigami 2.6 as Kirigami

Kirigami.LinkButton
{
    text: i18n("Show Dependencies...")

    onClicked: overlay.open()
    visible: view.model.count > 0

    Connections {
        target: resource
        onDependenciesFound: {
            view.model.clear()
            for (var v in dependencies) {
                view.model.append({display: i18n("<b>%1</b>: %2", v, dependencies[v])})
            }
        }
    }

    DiscoverPopup {
        id: overlay

        ListView {
            id: view
            anchors.fill: parent

            clip: true
            headerPositioning: ListView.OverlayHeader
            header: Kirigami.ItemViewHeader {
                title: i18n("%1 Dependencies", resource.name)
            }
            model: ListModel {}
            delegate: Kirigami.BasicListItem {
                width: parent.width
                text: modelData
            }
        }
    }
}
