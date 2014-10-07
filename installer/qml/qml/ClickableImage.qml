/*
    Copyright (C) 2011 Jonathan Thomas <echidnaman@kubuntu.org>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) version 3, or any
    later version accepted by the membership of KDE e.V. (or its
    successor approved by the membership of KDE e.V.), Nokia Corporation
    (or its successors, if any) and the KDE Free Qt Foundation, which shall
    act as a proxy defined in Section 6 of version 3 of the license.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library.  If not, see <http://www.gnu.org/licenses/>.
*/

import QtQuick 2.1
import MuonMobile 1.0

Image {
    id: image
    fillMode: Image.PreserveAspectFit
    asynchronous: true
    opacity: 0.0
    visible: true

    signal buttonClicked

    MouseArea {
        anchors.fill: parent
        acceptedButtons: Qt.LeftButton
        Component.onCompleted: clicked.connect(parent.buttonClicked)
    }

    MouseCursor {
        anchors.fill: parent
        shape: Qt.PointingHandCursor
    }

    transitions: Transition {
                    from: "*"
                    to: "loaded"
                    PropertyAnimation { target: image; property: "opacity"; duration: 500 }
                }

    states: [
        State {
            name: 'loaded'
            when: image.status == Image.Ready
            PropertyChanges { target: image; opacity: 1.0 }
        }
    ]
}
