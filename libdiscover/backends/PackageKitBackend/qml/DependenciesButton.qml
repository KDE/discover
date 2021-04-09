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
    visible: view.model.count > 0

    Connections {
        target: resource
        onDependenciesFound: {
            view.model.clear()
            for (var v in dependencies) {
                view.model.append(dependencies[v])
            }
        }
    }

    Kirigami.OverlaySheet {
        id: overlay

        parent: applicationWindow().overlay

        header: Kirigami.Heading { text: i18n("Dependencies for package: %1", resource.packageName) }

        ListView {
            id: view
            implicitWidth: Kirigami.Units.gridUnit * 25
            clip: true
            headerPositioning: ListView.OverlayHeader
            model: ListModel {}

            section.property: "packageInfo"
            section.delegate: Kirigami.ListSectionHeader {
                width: view.width
                height: Kirigami.Units.fontMetrics.xHeight * 4
                label: section
            }
            delegate: Kirigami.BasicListItem {
                width: view.width
                text: model.packageName
                subtitle: model.packageDescription
                // No need to offer a hover/selection effect since these list
                // items are non-interactive and non-selectable
                activeBackgroundColor: "transparent"
                activeTextColor: Kirigami.Theme.textColor
            }
        }
    }
}
