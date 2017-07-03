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

import QtQuick 2.0
import QtQuick.Layouts 1.2
import QtQuick.Templates 2.0 as T2
import QtGraphicalEffects 1.0
import org.kde.discover.app 1.0
import org.kde.kirigami 2.1 as Kirigami

T2.Control
{
    id: root
    property string search: ""
    property alias extra: extraLoader.sourceComponent
    property alias backgroundImage: actualHeader.backgroundImage

    anchors {
        left: parent.left
        right: parent.right
    }
    implicitHeight: actualHeader.implicitHeight + extraLoader.actualHeight
    z: actualHeader.z

    contentItem: Kirigami.ItemViewHeader {
        id: actualHeader
        view: root.ListView.view
        title: root.search.length>0 && page.title.length>0 ? i18n("Search: %1 + %2", root.search, page.title)
                                                             : root.search.length>0 ? i18n("Search: %1", root.search)
                                                             : page.title

        backgroundImage.asynchronous: false
    }

    bottomPadding: extraLoader.actualHeight

    Loader {
        id: extraLoader
        anchors {
            left: parent.left
            right: parent.right
            bottom: parent.bottom

            leftMargin: item ? item.anchors.leftMargin : 0
            rightMargin: item ? item.anchors.rightMargin : 0
            bottomMargin: item ? item.anchors.bottomMargin + distance : 0
        }
        property real distance: root.ListView.view.atYBeginning ? 0 : actualHeight
        Behavior on distance {
            PropertyAnimation {}
        }

        readonly property real actualHeight: item ? item.height + item.anchors.topMargin + item.anchors.bottomMargin : 0
        visible: item && distance<actualHeight
        Rectangle {
            color: Kirigami.Theme.backgroundColor
            anchors {
                fill: parent
                margins: -5
            }
        }
        sourceComponent: root.extra
    }
}
