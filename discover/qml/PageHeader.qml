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
import org.kde.discover.app 1.0
import org.kde.kirigami 1.0 as Kirigami

ColumnLayout {
    id: root
    readonly property QtObject _page: findPage()
    function findPage() {
        var obj = root;
        while(obj && !obj.hasOwnProperty("title")) {
            obj = obj.parent
        }
        return obj;
    }

    Label {
        id: titleLabel
        font.pointSize: SystemFonts.titleFont.pointSize * 3
        text: root._page.title
    }
    Rectangle {
        color: Kirigami.Theme.linkColor
        Layout.fillWidth: true
        height: 3
    }
}
