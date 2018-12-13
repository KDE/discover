/*
 *   Copyright (C) 2018 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
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

import QtQuick 2.8
import QtQuick.Controls 2.1
import org.kde.kirigami 2.1 as Kirigami

LinkButton
{
    text: i18n("Show Dependencies...")

    onClicked: overlay.open()
    visible: view.count > 0

    Connections {
        target: resource
        onDependenciesFound: {
            view.model.clear()
            for (var v in dependencies) {
                view.model.append({display: i18n("<b>%1</b>: %2", v, dependencies[v])})
            }
        }
    }

    DiscoverPopup {
        id: overlay

        ListView {
            id: view
            anchors.fill: parent

            clip: true
            headerPositioning: ListView.OverlayHeader
            header: Kirigami.ItemViewHeader {
                title: i18n("%1 Dependencies", resource.name)
            }
            model: ListModel {}
            delegate: Kirigami.BasicListItem {
                width: parent.width
                text: modelData
            }
        }
    }
}
