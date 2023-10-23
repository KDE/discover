/*
 *   SPDX-FileCopyrightText: 2021 Aleix Pol Gonzalez <aleixpol@kde.org>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.components 1 as Components

Components.Banner {
    // resource is set by the creator of the element in ApplicationPage.
    Layout.fillWidth: true
    height: visible ? implicitHeight : 0
    position: QQC2.ToolBar.Header
    text: resource.attentionText
    visible: resource && text.length > 0
    onLinkActivated: link => Qt.openUrlExternally(link)
}
