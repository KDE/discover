/***************************************************************************
 *   Copyright © 2012 Aleix Pol Gonzalez <aleixpol@blue-systems.com>       *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU General Public License as        *
 *   published by the Free Software Foundation; either version 2 of        *
 *   the License or (at your option) version 3 or any later version        *
 *   accepted by the membership of KDE e.V. (or its successor approved     *
 *   by the membership of KDE e.V.), which shall act as a proxy            *
 *   defined in Section 14 of version 3 of the license.                    *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>. *
 ***************************************************************************/

import QtQuick 2.1
import QtQuick.Controls 1.1
import QtQuick.Layouts 1.1
import QtQuick.Window 2.1
import org.kde.discover 1.0
import org.kde.discover.app 1.0
import "navigation.js" as Navigation

Rectangle
{
    color: DiscoverSystemPalette.base
    implicitHeight: 800
    implicitWidth: 900

    property Component currentTopLevel: null
    onCurrentTopLevelChanged: {
        if(currentTopLevel==null)
            return

        if(currentTopLevel.status==Component.Error) {
            console.log("status error: "+currentTopLevel.errorString())
        }
        while(stackView.depth>1) {
            var obj = stackView.pop()
            if(obj)
                obj.destroy(2000)
        }
        if(stackView.currentItem) {
            stackView.currentItem.destroy(2000)
        }
        stackView.replace(currentTopLevel, {}, window.status!=Component.Ready)
        window.clearSearch()
    }

    readonly property QtObject stack: stackView

    Rectangle {
        gradient: Gradient {
            GradientStop { position: 0.0; color: DiscoverSystemPalette.dark }
            GradientStop { position: 1.0; color: "transparent" }
        }
        height: parent.height/5
        anchors. fill: parent
        visible: !breadcrumbs.visible
    }
    Connections {
        target: app
        onPreventedClose: closePreventedInfo.enabled = true
    }

    ColumnLayout {
        spacing: 0
        anchors.fill: parent

        Repeater {
            model: MessageActionsModel {
                filterPriority: QAction.HighPriority
            }
            delegate: MessageAction {
                Layout.fillWidth: true
                height: Layout.minimumHeight
                theAction: action
            }
        }

        JustMessageAction {
            id: closePreventedInfo
            Layout.fillWidth: true
            message: i18n("Could not close the application, there are tasks that need to be done.")
        }

        JustMessageAction {
            Layout.fillWidth: true
            enabled: app.isRoot
            message: i18n("Running as <em>root</em> is discouraged and unnecessary.")
        }

        Breadcrumbs {
            id: breadcrumbs
            Layout.fillWidth: true
            visible: count>1

            pageStack: stackView
        }

        StackView {
            id: stackView
            Layout.fillWidth: true
            Layout.fillHeight: true

            onDepthChanged: {
                if (depth==1)
                    window.clearSearch()
            }
        }
    }
}
