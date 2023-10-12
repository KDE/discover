/*
 *   SPDX-FileCopyrightText: 2022 Aleix Pol Gonzalez <aleixpol@kde.org>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.12
import QtQml.Models 2.15
import org.kde.kirigami 2.14 as Kirigami
import org.kde.kirigami.delegates as KD

ColumnLayout {
    Kirigami.Heading {
        Layout.fillWidth: true
        text: i18ndc("libdiscover", "%1 is the name of the application", "Permissions for %1", resource.name)
        level: 2
        type: Kirigami.Heading.Type.Primary
        wrapMode: Text.Wrap
    }

    KD.SubtitleDelegate {
        Layout.fillWidth: true
        text: i18nd("libdiscover","Full Access")
        subtitle: i18nd("libdiscover", "Can access everything on the system")
        icon.name: "security-medium"

        // so that it gets neither hover nor pressed appearance
        hoverEnabled: false
        down: false
    }
}
