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
import QtQuick.Controls 1.1
import QtQuick.Dialogs 1.2
import QtQuick.Layouts 1.1

Dialog {
    id: newSourceDialog
    title: i18n("Specify the new source for %1", source.name)
    standardButtons: StandardButton.Ok | StandardButton.Close
    property QtObject source

    ColumnLayout {
        id: info
        anchors {
            left: parent.left
            right: parent.right
        }

        Label {
            id: description
            Layout.fillWidth: true
            Layout.fillHeight: true
            wrapMode: Text.WordWrap
            textFormat: Text.PlainText
            text: source.idDescription
        }

        TextField {
            id: repository
            Layout.fillWidth: true
            Keys.onEnterPressed: newSourceDialog.accept()
            focus: true
        }
    }
    onAccepted: source.addSource(repository.text)
}
