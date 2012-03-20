import QtQuick 1.1
import org.kde.plasma.components 0.1
import org.kde.qtextracomponents 0.1
import org.kde.muon 1.0
import "navigation.js" as Navigation

Item {
    property QtObject category
    property Item stack
    property alias sortRole: apps.stringSortRole
    property alias sortOrder: apps.sortOrder
    property int elemHeight: 65
    property alias stateFilter: apps.stateFilter
    property alias count: view.count
    property alias header: view.header
    property alias section: view.section

    function searchFor(text) { apps.search(text) }
    function stringToRole(role) { return apps.stringToRole(role) }
    function roleToString(role) { return apps.roleToString(role) }
    function applicationAt(i) { return apps.applicationAt(i) }
    
    ListView
    {
        id: view
        clip: true
        anchors {
            top: parent.top
            left: parent.left
            bottom: parent.bottom
            right: scroll.left
        }
        spacing: 3
        
        delegate: ListItem {
                width: view.width
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
                    anchors.top: icon.top
                    anchors.left: icon.right
                    anchors.right: ratingsItem.left
                    anchors.leftMargin: 5
                    font.pointSize: commentLabel.font.pointSize*1.7
                    elide: Text.ElideRight
                    text: name
                }
                Label {
                    id: commentLabel
                    anchors.bottom: icon.bottom
                    anchors.left: icon.right
                    anchors.leftMargin: 5
                    text: "<em>"+comment+"</em>"
                    opacity: delegateArea.containsMouse ? 1 : 0.2
                }
                Rating {
                    id: ratingsItem
                    anchors {
                        right: parent.right
                        top: parent.top
                    }
                    height: contHeight*.5
                    rating: model.rating
                }
                
                MouseArea {
                    id: delegateArea
                    anchors.fill: parent
                    onClicked: Navigation.openApplication(stack, application)
                    hoverEnabled: true
                    
                    InstallApplicationButton {
                        id: installButton
                        width: ratingsItem.width
                        height: contHeight*0.5
                        anchors {
                            bottom: parent.bottom
                            margins: 5
                        }
                        
                        property bool isVisible: delegateArea.containsMouse && !installButton.canHide
                        x: ratingsItem.x
                        opacity: isVisible ? 1 : 0
                        application: model.application
                        
                        Behavior on opacity {
                            NumberAnimation {
                                duration: 100
                                easing.type: Easing.InQuad
                            }
                        }
                    }
                }
            }
        
        model: ApplicationProxyModel {
            id: apps
            stringSortRole: "ratingPoints"
            sortOrder: Qt.DescendingOrder
            dynamicSortFilter: true
            
            Component.onCompleted: {
                if(category)
                    setFiltersFromCategory(category)
                sortModel()
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