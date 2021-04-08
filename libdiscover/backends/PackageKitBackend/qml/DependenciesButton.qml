/*
 *   SPDX-FileCopyrightText: 2018 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: LGPL-2.0-or-later
 */

import QtQuick 2.8
import QtQuick.Controls 2.1
import org.kde.kirigami 2.14 as Kirigami

Kirigami.LinkButton {
    text: i18n("Show Dependencies...")

    onClicked: overlay.open()
    visible: view.count > 0

    Connections {
        target: resource
        onDependenciesFound: {
            view.model.clear()
            for (var v in dependencies) {
                view.model.append({display: i18n("<b>%1</b>: %2", v, dependencies[v])})
            }
        }
    }

    Kirigami.OverlaySheet {
        id: overlay

        parent: applicationWindow().overlay

        header: Kirigami.Heading { text: i18n("Package dependencies for %1", resource.name) }

        ListView {
            id: view

            implicitWidth: Kirigami.Units.gridUint * 20

            clip: true
            headerPositioning: ListView.OverlayHeader
            model: ListModel {}
            delegate: Kirigami.BasicListItem {
                width: view.width
                text: modelData
            }
        }
    }
}
