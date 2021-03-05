/*
 *   SPDX-FileCopyrightText: 2014 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: LGPL-2.0-or-later
 */

import QtQuick 2.1
import QtQuick.Controls 2.1
import QtQuick.Layouts 1.1
import org.kde.kirigami 2.14 as Kirigami

Kirigami.OverlaySheet
{
    id: newSourceDialog

    property string displayName
    property QtObject source

    header: Kirigami.Heading {
        text: i18n("Add New %1 Repository", displayName)
        wrapMode: Text.WordWrap
    }

    onSheetOpenChanged: {
        if (sheetOpen) {
            repository.forceActiveFocus();
        }
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
            onAccepted: okButton.clicked()
            focus: true
            onTextChanged: color = Kirigami.Theme.textColor
        }

        DialogButtonBox {
            Layout.topMargin: Kirigami.Units.largeSpacing
            Layout.fillWidth: true

            // Cancel out built-in margins so it lines up with the rest of the
            // content in this sheet
            Layout.margins: -Kirigami.Units.smallSpacing

            Button {
                id: okButton
                DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
                text: i18n("Add")
                icon.name: "list-add"
                onClicked: if (source.addSource(repository.text)) {
                    newSourceDialog.close()
                } else {
                    repository.color = Kirigami.Theme.negativeTextColor
                }
            }

            Button {
                id: cancelButton
                DialogButtonBox.buttonRole: DialogButtonBox.DestructiveRole
                text: i18n("Cancel")
                icon.name: "dialog-cancel"
                onClicked: newSourceDialog.close()
            }
        }
    }
}
