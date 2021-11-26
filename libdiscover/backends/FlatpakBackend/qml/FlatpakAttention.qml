/*
 *   SPDX-FileCopyrightText: 2021 Aleix Pol Gonzalez <aleixpol@kde.org>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

import QtQuick 2.15
import QtQuick.Layouts 1.1
import org.kde.kirigami 2.10 as Kirigami

Kirigami.InlineMessage
{
    // resource is set by the creator of the element in ApplicationPage.
    //required property AbstractResource resource
    Layout.fillWidth: true
    text: resource.attentionText
    visible: resource && text.length > 0
    onLinkActivated: Qt.openUrlExternally(link)
}
