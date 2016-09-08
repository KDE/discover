/*
 *   Copyright (C) 2015 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
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

import QtQuick 2.5
import org.kde.kirigami 1.0 as Kirigami

Kirigami.ScrollablePage
{
    id: root

    /**
     * pageHeader: Component
     * Places an item on top of the view when the user scrolls, so that we can
     * have some UI available at all time.
     */
    property alias pageHeader: pageHeaderLoader.sourceComponent
    property alias pageOverlay: overlayLoader.sourceComponent

    readonly property Item overlay: Item {
        parent: root
        anchors.fill: parent

        Loader {
            id: overlayLoader
            anchors.fill: parent
        }

        Loader {
            id: pageHeaderLoader
            anchors {
                left: parent.left
                right: parent.right
                rightMargin: root.flickable ? root.width - root.contentItem.width : 0
            }

            Behavior on y {
                NumberAnimation {
                    duration: Kirigami.Units.longDuration
                    easing.type: Easing.InOutQuad
                }
            }

            states: [
                State {
                    name: "top"
                    when: root.flickable.contentY > 0
                    PropertyChanges { target: pageHeaderLoader; y: 0 }
                },
                State {
                    name: "scrolled"
                    when: root.flickable.contentY <= 0
                    PropertyChanges { target: pageHeaderLoader; y: -height }
                }
            ]
        }
    }
}
