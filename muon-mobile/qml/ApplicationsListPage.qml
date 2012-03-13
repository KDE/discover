import QtQuick 1.0
import org.kde.plasma.components 0.1

Page {
    id: page
    property alias category: apps.category
    property alias sortRole: apps.sortRole
    property alias stateFilter: apps.stateFilter
    
    tools: Item {
        opacity: page.status == PageStatus.Active ? 1 : 0
        height: field.height
        
        TextField {
            id: field
            anchors {
                verticalCenter: parent.verticalCenter
                right: button.left
                left: parent.left
                rightMargin: 5
            }
            placeholderText: i18n("Search...")
            onTextChanged: apps.searchFor(text)
        }
        MuonToolButton {
            id: button
            icon: "view-sort-ascending"
            anchors.verticalCenter: parent.verticalCenter
            anchors.right: parent.right
            checkable: true
            checked: sortMenu.visible
            onClicked: sortMenu.visible=!sortMenu.visible
            
            Item {
                id: sortMenu
                width: 100
                height: buttons.height
                anchors.right: parent.right
                anchors.top: parent.bottom
                visible: false
                Rectangle {
                    anchors.fill: parent
                    radius: 10
                    opacity: 0.4
                }
                
                Column {
                    id: buttons
                    width: parent.width
                    
                    Repeater {
                        model: paramModel
                        delegate: ToolButton {
                            width: buttons.width
                            text: display
                            onClicked: apps.sortRole=apps.stringToRole(role)
                            checked: apps.roleToString(apps.sortRole)==role
                        }
                    }
                }
            }
        }
    }
    
    property list<QtObject> paramModel: [
        QtObject {
            property string display: i18n("Name")
            property string role: "name"
        },
        QtObject {
            property string display: i18n("Rating")
            property string role: "rating"
        }
    ]
    
    ApplicationsList {
        id: apps
        anchors.fill: parent
        stack: page.pageStack
    }
}