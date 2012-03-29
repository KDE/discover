import QtQuick 1.1
import org.kde.plasma.components 0.1
import org.kde.qtextracomponents 0.1
import org.kde.muon 1.0
import "navigation.js" as Navigation

Item {
    id: parentItem
    property Item stack
    property alias sortRole: apps.stringSortRole
    property alias sortOrder: apps.sortOrder
    property int elemHeight: 65
    property alias stateFilter: apps.stateFilter
    property alias count: view.count
    property alias header: view.header
    property string section//: view.section
    property alias category: apps.filteredCategory
    property bool preferUpgrade: false

    function searchFor(text) { apps.search(text); apps.sortOrder=Qt.AscendingOrder }
    function stringToRole(role) { return apps.stringToRole(role) }
    function roleToString(role) { return apps.roleToString(role) }
    function applicationAt(i) { return apps.applicationAt(i) }
    
    GridView
    {
        id: view
        clip: true
        property int minCellWidth: 200
        cellWidth: view.width/Math.floor(view.width/minCellWidth)-1
        cellHeight: cellWidth/1.618 //tau
        anchors {
            top: parent.top
            left: parent.left
            bottom: parent.bottom
            right: scroll.left
            leftMargin: parent.width/8
            rightMargin: parent.width/8
        }
        
        delegate: ListItem {
                width: view.cellWidth-5
                height: view.cellHeight
                property real contHeight: view.cellHeight*0.7
//                 Image {
//                     id: icon
//                     width: parent.width; height: contHeight
//                     source: model.application.screenshotUrl(1)
//                     asynchronous: true
//                     cache: true
//                     smooth: true
//                     fillMode: Image.PreserveAspectFit
//                     anchors {
//                         horizontalCenter: parent.horizontalCenter
//                         top: parent.top
//                     }
//                 }
                
                QIconItem {
                    id: icon
                    anchors {
                        horizontalCenter: parent.horizontalCenter
                        top: parent.top
                        topMargin: 5
                    }
                    icon: model["icon"];
                    width: parent.width; height: contHeight
                }
                Label {
                    anchors {
                        top: icon.bottom
                        left: parent.left
                        right: parent.right
                        leftMargin: 5
                    }
                    font.pointSize: 13
                    horizontalAlignment: Text.AlignHCenter
                    elide: Text.ElideRight
                    text: name
                }
//                 Label {
//                     id: commentLabel
//                     anchors.bottom: icon.bottom
//                     anchors.left: icon.right
//                     anchors.leftMargin: 5
//                     text: "<em>"+comment+"</em>"
//                     opacity: delegateArea.containsMouse ? 1 : 0.2
//                 }
//                 Rating {
//                     id: ratingsItem
//                     anchors {
//                         right: parent.right
//                         top: parent.top
//                     }
//                     height: contHeight*.5
//                     rating: model.rating
//                 }
                
                MouseArea {
                    id: delegateArea
                    anchors.fill: parent
                    onClicked: Navigation.openApplication(stack, application)
                    hoverEnabled: true
                    
//                     InstallApplicationButton {
//                         id: installButton
//                         width: ratingsItem.width
//                         height: contHeight*0.5
//                         anchors {
//                             bottom: parent.bottom
//                             margins: 5
//                         }
//                         
//                         property bool isVisible: delegateArea.containsMouse && !installButton.canHide
//                         x: ratingsItem.x
//                         opacity: isVisible ? 1 : 0
//                         application: model.application
//                         preferUpgrade: parentItem.preferUpgrade
//                         
//                         Behavior on opacity {
//                             NumberAnimation {
//                                 duration: 100
//                                 easing.type: Easing.InQuad
//                             }
//                         }
//                     }
                }
            }
        
        model: ApplicationProxyModel {
            id: apps
            stringSortRole: "ratingPoints"
            sortOrder: Qt.DescendingOrder
            
            Component.onCompleted: sortModel()
        }
    }
    
    ScrollBar {
        id: scroll
        orientation: Qt.Vertical
        flickableItem: view
        stepSize: 40
        scrollButtonInterval: 50
        anchors {
                top: parent.top
                right: parent.right
                bottom: parent.bottom
        }
    }
}