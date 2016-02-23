/*
 *   Copyright (C) 2012 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
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
import QtQuick.Layouts 1.1
import QtQuick.Window 2.1
import org.kde.discover.app 1.0 as Discover
import org.kde.discover 1.0

GridLayout
{
    id: layout
    property QtObject application

    columns: 2

    Heading {
        Layout.fillWidth: true
        text: i18n("Details")
    }
    Item { width: 5; height: 5 }//if the heading has columnSpan: 2 the whole layout shrinks

    Repeater {
        model: details
        property list<QtObject> details: [
            QtObject {
                readonly property string title: i18n("Homepage")
                readonly property string value: "<a href='"+application.homepage+"'>"+application.homepage+"</a>"
                readonly property int span: 2
                readonly property var url: application.homepage
            },
            QtObject {
                readonly property string title: i18n("Size")
                readonly property string value: application.sizeDescription
            },
            QtObject {
                readonly property string title: i18n("License")
                readonly property string value: application.license
            },
            QtObject {
                readonly property string title: i18n("Version")
                readonly property string value: application.isInstalled ? application.installedVersion : application.availableVersion
            }
        ]
        delegate: Column {
            Layout.fillWidth: true
            Layout.columnSpan: modelData.span ? modelData.span : 1
            spacing: 0
            Label {
                text: modelData.title
            }
            Label {
                text: modelData.value
                elide: Text.ElideRight
                opacity: modelData.url ? 1 : 0.5
                onLinkActivated: if (modelData.url) {
                    Qt.openUrlExternally(modelData.url);
                }
            }
        }
    }
}
