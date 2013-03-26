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

import QtQuick 1.1
import org.kde.plasma.core 0.1
import org.kde.plasma.components 0.1
import org.kde.qtextracomponents 0.1
import org.kde.muon 1.0

Page
{
    id: page
    property QtObject application
    
    Item {
        id: intro
        anchors {
            top: parent.top
            left: parent.left
            right: parent.right
            margins: 10
        }
        height: icon.height
        IconItem {
            id: icon
            anchors.top: parent.top
            anchors.left: parent.left
            width: 40
            height: 40
            
            source: application.icon
        }
        
        Column {
            id: header
            anchors.top: parent.top
            anchors.left: icon.right
            anchors.right: parent.right
            anchors.leftMargin: 5
            spacing: 5
            
            Text {
                text: application.name
                width: parent.width
                font.bold: true
            }
            Label {
                text: application.comment
                wrapMode: Text.WordWrap
                width: parent.width
            }
        }
        
        InstallApplicationButton {
            id: installButton
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            application: page.application
        }
    }
    
    TabBar {
        id: tabs
        anchors {
            top: intro.bottom
            left: parent.left
            right: parent.right
            margins: 10
        }
        TabButton {
            tab: applicationOverview
            text: i18n("Overview")
        }
        TabButton {
            tab: addonsView
            text: i18n("Add-ons")
            visible: !addonsView.isEmpty //TODO: probably should be enabled: instead
        }
        TabButton {
            tab: reviewsView
            text: i18n("Reviews")
            visible: reviewsView.hasReviews
        }
    }
    
    TabGroup {
        currentTab: tabs.currentTab.tab
        anchors {
            top: tabs.bottom
            left: parent.left
            right: parent.right
            bottom: parent.bottom
            margins: 10
        }
        
        ApplicationOverview {
            id: applicationOverview
            application: page.application
        }
        
        AddonsView {
            id:addonsView
            application: page.application
            isInstalling: installButton.isActive
        }
        
        ReviewsView {
            id: reviewsView
            application: page.application
        }
    }
}
