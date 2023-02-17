/*
 *   SPDX-FileCopyrightText: 2022 Aleix Pol Gonzalez <aleixpol@kde.org>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

import org.kde.kirigami 2.19 as Kirigami
import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtWebView 1.15

Kirigami.OverlaySheet {
    id: sheet

    property QtObject transaction
    property bool acted: false
    property alias url: view.url

    showCloseButton: false

    readonly property var p0: Connections {
        target: transaction
        function onWebflowDone() {
            acted = true
            sheet.close()
        }
    }

    contentItem: WebView {
        id: view
        Layout.preferredWidth: Math.round(window.width * 0.75)
        height: Math.round(window.height * 0.75)
    }

    footer: RowLayout {
        Item { Layout.fillWidth : true }

        Button {
            Layout.alignment: Qt.AlignRight
            text: i18n("Cancel")
            icon.name: "dialog-cancel"
            onClicked: {
                transaction.cancel()
                sheet.acted = true
                sheet.close()
            }
            Keys.onEscapePressed: clicked()
        }
    }

    onSheetOpenChanged: if (!sheetOpen) {
        sheet.destroy(1000)
        if (!sheet.acted) {
            transaction.cancel()
        }
    }
}
