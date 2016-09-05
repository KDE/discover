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
import org.kde.kirigami 1.0 as Kirigami

ColumnLayout {
    id: root
    readonly property QtObject _page: findPage()
    property string background
    property string search: ""
    readonly property bool shadow: background.length > 0

    function findPage() {
        var obj = root;
        while(obj && !obj.hasOwnProperty("title")) {
            obj = obj.parent
        }
        return obj;
    }
    spacing: 0

    Component.onCompleted: {
        if (!root._page.pageHeader)
            root._page.pageHeader = tinyHeader
    }

    Component {
        id: tinyHeader
        Item {
            height: layout.implicitHeight
            DropShadow {
                anchors.fill: bg
                source: bg
                horizontalOffset: 0
                verticalOffset: 3
                radius: 8.0
                samples: 17
                color: "gray"
            }
            Image {
                id: bg
                anchors.fill: parent
                source: root.background
                fillMode: Image.PreserveAspectCrop
            }

            RowLayout {
                id: layout
                anchors {
                    left: parent.left
                    right: parent.right
                }
                Item {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                }
                LinkButton {
                    shadow: root.background !== ""
                    text: titleLabel.text
                    onClicked: {
                        var flic = root._page.flickable
                        if (flic.positionViewAtBeginning)
                            flic.positionViewAtBeginning();
                        else
                            flic.contentY = 0;
                    }
                }
            }
        }
    }

    Image {
        id: decorationImage
        fillMode: Image.PreserveAspectCrop
        Layout.preferredHeight: root.shadow ? titleLabel.paintedHeight * 4 : titleLabel.paintedHeight * 2
        Layout.fillWidth: true
        source: root.background

        Label {
            id: titleLabel
            anchors {
                fill: parent
                margins: Kirigami.Units.gridUnit
            }
            font.pointSize: SystemFonts.titleFont.pointSize * 3
            text: root.search.length>0 && root._page.title.length>0 ? i18n("Search: %1 + %2", root.search, root._page.title)
                : root.search.length>0 ? i18n("Search: %1", root.search)
                : root._page.title
            color: root.shadow ? Kirigami.Theme.highlightedTextColor : Kirigami.Theme.linkColor
            horizontalAlignment: Text.AlignRight
            verticalAlignment: Text.AlignBottom
        }

        DropShadow {
            horizontalOffset: 3
            verticalOffset: 3
            radius: 8.0
            samples: 17
            color: "#80000000"
            source: titleLabel
            anchors.fill: titleLabel
            visible: root.shadow
        }
    }
    Rectangle {
        color: Kirigami.Theme.linkColor
        Layout.fillWidth: true
        height: 3
    }
}
