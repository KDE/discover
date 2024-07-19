/*
 *   SPDX-FileCopyrightText: 2022 Aleix Pol Gonzalez <aleixpol@kde.org>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

import QtQuick
import QtQuick.Layouts
import org.kde.discover as Discover
import org.kde.kirigami as Kirigami

Kirigami.InlineMessage {
    required property Discover.AbstractResource resource
    Discover.Activatable.active: text.length > 0

    Layout.fillWidth: true
    text: resource.eolReason
    height: visible ? implicitHeight : 0
    type: Kirigami.MessageType.Warning
}
