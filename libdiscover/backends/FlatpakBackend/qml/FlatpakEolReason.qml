/*
 *   SPDX-FileCopyrightText: 2022 Aleix Pol Gonzalez <aleixpol@kde.org>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.components 1 as Components
import org.kde.discover
import org.kde.discover.app

Components.Banner {
    // resource is set by the creator of the element in ApplicationPage.
    Layout.fillWidth: true
    height: visible ? implicitHeight : 0
    text: resource.eolReason
    position: QQC2.ToolBar.Header
    visible: text.length > 0
    type: Kirigami.MessageType.Warning
}
