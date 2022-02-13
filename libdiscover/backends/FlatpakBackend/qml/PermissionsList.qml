/*
 *   SPDX-FileCopyrightText: 2022 Suhaas Joshi <joshiesuhaas0@gmail.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

import QtQuick 2.0
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.12
import QtQml.Models 2.15
import org.kde.kirigami 2.14 as Kirigami

Item {
    id: root
    implicitWidth: Kirigami.Units.gridUnit * 35
    implicitHeight: permissionsColumn.height + Kirigami.Units.gridUnit * 2
    visible: list.model.rowCount() > 0

    ColumnLayout {
        id: permissionsColumn
        width: root.width

        Kirigami.Heading {
            text: i18nc("%1 is the name of the application", "Permissions for %1", resource.name)
            font.weight: Font.DemiBold
            level: 2
            Layout.fillWidth: true
            wrapMode: Text.Wrap
        }

        Instantiator {
            id: list
            model: resource.showPermissions()

            Kirigami.BasicListItem {
                parent: permissionsColumn
                text: model.brief
                subtitle: model.description
                icon: model.icon
                hoverEnabled: false
                width: permissionsColumn.width
            }
        }
    }
}
