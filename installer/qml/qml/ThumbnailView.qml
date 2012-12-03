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

import QtQuick 1.0
import Effects 1.0

Rectangle {
    id: rectangle1
    width: 170
    height: 130

    property alias source: image.source
    signal thumbnailClicked()
    signal thumbnailLoaded()

    ClickableImage {
        id: image
        width: 160
        height: 120

        effect: DropShadow {
             blurRadius: 10
        }

        onStatusChanged: {
            if (image.status == Image.Error)
                view.hide()
            else if (image.status == Image.Ready)
                parent.thumbnailLoaded()
        }
        onButtonClicked: parent.thumbnailClicked()
    }
}
