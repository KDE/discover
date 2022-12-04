/*
 *   SPDX-FileCopyrightText: 2014 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: LGPL-2.0-or-later
 */

import QtQuick 2.1
import QtQuick.Controls 2.1
import QtQuick.Layouts 1.1
import org.kde.kirigami 2.20 as Kirigami

Kirigami.PromptDialog
{
    id: newSourceDialog
    preferredWidth: Kirigami.Units.gridUnit * 20

    property string displayName
    property QtObject source

    title: i18n("Add New %1 Repository", displayName)

    onVisibleChanged: {
        if (visible) {
            repository.forceActiveFocus();
        }
    }
    
    standardButtons: Kirigami.Dialog.Ok | Kirigami.Dialog.Cancel
    
    onAccepted: {
        if (source.addSource(repository.text)) {
            newSourceDialog.close()
        } else {
            repository.color = Kirigami.Theme.negativeTextColor
        }
    }
    
    onRejected: {
        newSourceDialog.close()
    }

    ColumnLayout {
        Label {
            Layout.fillWidth: true
            wrapMode: Text.Wrap
            textFormat: Text.PlainText
            text: source.idDescription
        }

        TextField {
            id: repository
            Layout.fillWidth: true
            onAccepted: newSourceDialog.accept()
            focus: true
            onTextChanged: color = Kirigami.Theme.textColor
        }
    }
}
