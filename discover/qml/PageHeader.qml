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
import QtQuick.Controls 1.2
import QtGraphicalEffects 1.0
import org.kde.discover.app 1.0
import org.kde.kirigami 2.0 as Kirigami

Rectangle {
    id: root
    readonly property QtObject _page: findPage()
    property string background
    property string search: ""
    property Flickable view: null
    color: Kirigami.Theme.backgroundColor

    property alias extra: extraLoader.sourceComponent

    function findPage() {
        var obj = root;
        while(obj && !obj.hasOwnProperty("title")) {
            obj = obj.parent
        }
        return obj;
    }
    z: 500
    readonly property real initialHeight: decorationImage.Layout.preferredHeight + rect.height + bottomPadding

    function boundHeight(min, scrolledDistance, max) {
        return Math.max(max-Math.max(0, scrolledDistance), min)
    }

    height: view ? boundHeight(Kirigami.Units.largeSpacing*2, view.contentY/2, initialHeight) : 0

    anchors {
        left: parent.left
        right: parent.right
    }

    Image {
        id: decorationImage
        anchors {
            left: parent.left
            right: parent.right
            top: parent.top
            bottom: rect.top
        }
        z: 501
        readonly property bool shadow: background.length > 0 && decorationImage.status !== Image.Error
        fillMode: Image.PreserveAspectCrop
        Layout.minimumHeight: Kirigami.Units.largeSpacing
        Layout.preferredHeight: (decorationImage.shadow ? 10 : 6) * Kirigami.Units.largeSpacing
        Layout.fillWidth: true
        source: root.background

        Label {
            id: titleLabel
            anchors {
                fill: parent
                rightMargin: Kirigami.Units.gridUnit
            }
            text: root.search.length>0 && root._page.title.length>0 ? i18n("Search: %1 + %2", root.search, root._page.title)
                : root.search.length>0 ? i18n("Search: %1", root.search)
                : root._page.title
            color: decorationImage.shadow ? Kirigami.Theme.highlightedTextColor : Kirigami.Theme.linkColor
            horizontalAlignment: Text.AlignRight
            verticalAlignment: Text.AlignBottom
            elide: Text.ElideRight

            font.pixelSize: Math.min(SystemFonts.titleFont.pixelSize * 3, root.height/2)
        }

        DropShadow {
            horizontalOffset: 3
            verticalOffset: 3
            radius: 8.0
            samples: 17
            color: "#80000000"
            source: titleLabel
            anchors.fill: titleLabel
            visible: decorationImage.shadow
        }
    }
    Rectangle {
        id: rect
        color: root._page.isCurrentPage ? Kirigami.Theme.linkColor : Kirigami.Theme.disabledTextColor
        height: 3
        z: 501
        anchors {
            left: parent.left
            right: parent.right
            bottom: parent.bottom
            bottomMargin: extraLoader.item ? Math.max(0, extraLoader.item.height + extraLoader.item.anchors.topMargin + extraLoader.item.anchors.bottomMargin - Math.max(0, view.contentY/2)) : 0
        }
    }

    Loader {
        id: extraLoader
        anchors {
            left: parent.left
            right: parent.right
            bottom: parent.bottom

            leftMargin: item ? item.anchors.leftMargin : 0
            rightMargin: item ? item.anchors.rightMargin : 0
            bottomMargin: item ? item.anchors.bottomMargin : 0
        }
        sourceComponent: root.extra
    }
}
