/*
 *   Copyright (C) 2014 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library/Lesser General Public License
 *   version 2, or (at your option) any later version, as published by the
 *   Free Software Foundation
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details
 *
 *   You should have received a copy of the GNU Library/Lesser General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

import QtQuick 2.1
import QtQuick.Controls 2.1
import QtQuick.Layouts 1.1
import org.kde.kirigami 2.10 as Kirigami

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
            Layout.fillWidth: true

            // Cancel out built-in margins so it lines up with the rest of the
            // content in this sheet
            Layout.margins: -units.smallSpacing

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
