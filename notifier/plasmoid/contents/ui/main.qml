/***************************************************************************
 *   Copyright (C) 2014 by Aleix Pol Gonzalez <aleixpol@blue-systems.com>  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA .        *
 ***************************************************************************/

import QtQuick 2.2
import org.kde.plasma.plasmoid 2.0
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.muonnotifier 1.0

Item
{
    Plasmoid.fullRepresentation: Full {}

    Binding {
        target: plasmoid
        property: "status"
        value: switch (MuonNotifier.state) {
                case MuonNotifier.NoUpdates:
                    return PlasmaCore.Types.PassiveStatus;
                case MuonNotifier.NormalUpdates:
                case MuonNotifier.SecurityUpdates:
                    return PlasmaCore.Types.ActiveStatus;
            }
    }

    Component.onCompleted: {
        plasmoid.setAction("update", i18n("See Updates..."), "system-software-update");
    }

    function action_update() {
        MuonNotifier.showMuon();
    }

    Plasmoid.compactRepresentation: PlasmaCore.IconItem {
        source: MuonNotifier.iconName
        MouseArea {
            anchors.fill: parent
            onClicked: plasmoid.expanded = !plasmoid.expanded
        }
    }
}
