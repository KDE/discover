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
import QtQml.Models 2.3
import org.kde.plasma.plasmoid 2.0
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.discovernotifier 1.0

Item
{
    id: root
    Plasmoid.icon: DiscoverNotifier.iconName
    Plasmoid.toolTipSubText: DiscoverNotifier.message
    Plasmoid.status: {
        switch (DiscoverNotifier.state) {
        case DiscoverNotifier.NoUpdates:
            return PlasmaCore.Types.PassiveStatus;
        case DiscoverNotifier.NormalUpdates:
        case DiscoverNotifier.SecurityUpdates:
        case DiscoverNotifier.RebootRequired:
            return PlasmaCore.Types.ActiveStatus;
        }
    }

    Plasmoid.onActivated: action_discover()

    Plasmoid.preferredRepresentation: Plasmoid.compactRepresentation
    Plasmoid.compactRepresentation: PlasmaCore.IconItem {
        source: plasmoid.icon

        MouseArea {
            anchors.fill: parent
            onClicked: action_update()
        }
    }

    Component.onCompleted: {
        plasmoid.setAction("discover", i18n("Open Software Center..."), "plasma-discover");
        plasmoid.setAction("update", i18n("See Updates..."), "system-software-update");
        plasmoid.setAction("refresh", i18n("Refresh..."), "view-refresh");
        if (DiscoverNotifier.needsReboot)
            plasmoid.setAction("reboot", i18n("Restart..."), "system-reboot");
    }

    Connections {
        target: DiscoverNotifier
        onNeedsRebootChanged: plasmoid.setAction("reboot", i18n("Restart..."), "system-reboot");
    }

    Instantiator {
        Connections {
            target: DiscoverNotifier
            onNewUpgradeAction: {
                upgrades.append(action)
            }
        }

        model: ObjectModel {
            id: upgrades
        }

        onObjectAdded: {
            plasmoid.setAction(index, object.description, "system-upgrade")
        }
    }

    function actionTriggered(actionName) {
        var index = parseInt(actionName);
        if (index)
            upgrades.get(index).trigger()
    }

    function action_discover() {
        DiscoverNotifier.showDiscover();
    }
    function action_update() {
        DiscoverNotifier.showDiscoverUpdates();
        root.activated()
    }
    function action_refresh() {
        DiscoverNotifier.recheckSystemUpdateNeeded();
    }

    function action_reboot() {
        DiscoverNotifier.reboot()
    }
}
