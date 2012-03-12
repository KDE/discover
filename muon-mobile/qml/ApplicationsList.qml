import QtQuick 1.1
import org.kde.plasma.components 0.1
import org.kde.qtextracomponents 0.1
import org.kde.muon 1.0
import "navigation.js" as Navigation

Item {
    property QtObject category
    property alias sortRole: apps.sortRole
    property int elemHeight: 40
    property alias stateFilter: apps.stateFilter
    property alias count: view.count

    function searchFor(text) {
        apps.search(text)
    }
    
    function stringToRole(role) { return apps.stringToRole(role) }
    function roleToString(role) { return apps.roleToString(role) }
    
    ListView
    {
        id: view
        anchors {
            top: parent.top
            left: parent.left
            bottom: parent.bottom
            right: scroll.left
        }
        Component {
            id: delegate
            ListItem {
                property real contHeight: elemHeight*0.7
                height: elemHeight
                QIconItem {
                    id: icon
                    icon: model["icon"]; width: contHeight; height: contHeight
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.left: parent.left
                }
                
                QIconItem {
                    anchors.right: icon.right
                    anchors.bottom: icon.bottom
                    visible: installed
                    icon: "dialog-ok"
                    height: 16
                    width: 16
                }
                Label {
                    anchors.top: parent.top
                    anchors.left: icon.right
                    anchors.leftMargin: 5
                    anchors.topMargin: -5
                    text: name
                }
                Label {
                    anchors.bottom: parent.bottom
                    anchors.left: icon.right
                    anchors.leftMargin: 5
                    anchors.bottomMargin: -5
                    text: "<em>"+comment+"</em>"
                    opacity: delegateArea.containsMouse ? 1 : 0.2
                }
                Rating {
                    anchors.right: parent.right
                    anchors.top: parent.top
                    rating: model["rating"]
                    height: contHeight*.7
                }
                
                MouseArea {
                    id: delegateArea
                    anchors.fill: parent
                    onClicked: Navigation.openApplication(application)
                    hoverEnabled: true
                }
            }
        }
        delegate: delegate
        
        model: ApplicationProxyModel {
            id: apps
            sortRole: 37
            dynamicSortFilter: true
            
            Component.onCompleted: {
                if(category)
                    setFiltersFromCategory(category)
                sortModel(0, Qt.DescendingOrder)
            }
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