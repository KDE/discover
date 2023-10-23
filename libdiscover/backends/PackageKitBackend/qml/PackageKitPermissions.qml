/*
 *   SPDX-FileCopyrightText: 2022 Aleix Pol Gonzalez <aleixpol@kde.org>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

import QtQuick
import QtQuick.Layouts
import org.kde.discover as Discover
import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.formcard as FormCard

ColumnLayout {
    id: root

    required property Discover.AbstractResource resource
    Discover.Activatable.active: true

    spacing: 0

    FormCard.FormHeader {
        title: i18ndc("libdiscover", "Permission to access system resources and hardware devices", "Permissions")
    }

    FormCard.FormCard {
        FormCard.FormTextDelegate {
            text: i18nd("libdiscover","Full Access")
            description: i18nd("libdiscover", "Can access everything on the system")
            icon.name: "security-medium"
        }
    }
}
