/*
 *   SPDX-FileCopyrightText: 2022 Aleix Pol Gonzalez <aleixpol@kde.org>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

import QtQuick 2.15
import QtQuick.Layouts 1.1
import org.kde.kirigami 2.10 as Kirigami
import org.kde.discover 2.0
import org.kde.discover.app 1.0
import "navigation.js" as Navigation

Kirigami.InlineMessage
{
    // resource is set by the creator of the element in ApplicationPage.
    //required property AbstractResource resource
    Layout.fillWidth: true
    text: resource.eolReason
    height: visible ? implicitHeight : 0
    visible: text.length > 0
    type: Kirigami.MessageType.Warning
}
