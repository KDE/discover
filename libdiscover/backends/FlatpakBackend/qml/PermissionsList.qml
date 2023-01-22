/*
 *   SPDX-FileCopyrightText: 2022 Suhaas Joshi <joshiesuhaas0@gmail.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.12
import QtQml.Models 2.15
import org.kde.kirigami 2.14 as Kirigami

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
            hoverEnabled: false
        }
    }
}
