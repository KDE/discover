/*
 *   SPDX-FileCopyrightText: 2014 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: LGPL-2.0-or-later
 */

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.discover as Discover

Kirigami.PromptDialog {
    id: root

    preferredWidth: Kirigami.Units.gridUnit * 20

    required property string displayName
    required property Discover.AbstractSourcesBackend source

    title: i18n("Add New %1 Repository", displayName)

    onVisibleChanged: {
        if (visible) {
            repository.forceActiveFocus();
        }
    }

    standardButtons: QQC2.Dialog.NoButton

    onAccepted: {
        if (source.addSource(repository.text)) {
            close()
        } else {
            repository.color = Kirigami.Theme.negativeTextColor
        }
    }

    onRejected: {
        close()
    }

    customFooterActions: [
        Kirigami.Action {
            text: i18n("Add")
            icon.name: "list-add"
            onTriggered: root.accept();
        },
        Kirigami.Action {
            text: i18n("Cancel")
            icon.name: "dialog-cancel"
            onTriggered: root.reject();
        }
    ]

    ColumnLayout {
        QQC2.Label {
            Layout.fillWidth: true
            wrapMode: Text.Wrap
            textFormat: Text.PlainText
            text: root.source.idDescription
        }

        QQC2.TextField {
            id: repository
            Layout.fillWidth: true
            onAccepted: root.accept()
            focus: true
            onTextChanged: color = Kirigami.Theme.textColor
        }
    }
}
