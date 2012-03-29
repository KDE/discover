import QtQuick 1.0
import org.kde.plasma.components 0.1

Page {
    id: page
    property alias category: apps.category
    property alias sortRole: apps.sortRole
    property alias sortOrder: apps.sortOrder
    property alias stateFilter: apps.stateFilter
    property alias section: apps.section
    
    function searchFor(text) {
        field.text = text
        field.focus = true
    }
    
    tools: Item {
        opacity: page.status == PageStatus.Active ? 1 : 0
        height: field.height
        
        TextField {
            id: field
            anchors {
                verticalCenter: parent.verticalCenter
                right: buttonsRow.left
                left: parent.left
                rightMargin: 5
            }
            placeholderText: i18n("Search...")
            onTextChanged: apps.searchFor(text)
        }
        Row {
            id: buttonsRow
            anchors.right: parent.right
            MuonToolButton {
                id: button
                icon: "view-sort-ascending"
                anchors.verticalCenter: parent.verticalCenter
                
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
                                onClicked: {
                                    apps.sortRole=role
                                    apps.sortOrder=sorting
                                }
                                checked: apps.sortRole==role
                            }
                        }
                    }
                }
            }
            
            MuonToolButton {
                id: listViewShown
                checkable: true
                icon: "tools-wizard"
            }
        }
    }
    
    property list<QtObject> paramModel: [
        QtObject {
            property string display: i18n("Name")
            property string role: "name"
            property variant sorting: Qt.AscendingOrder
        },
        QtObject {
            property string display: i18n("Rating")
            property string role: "sortableRating"
            property variant sorting: Qt.DescendingOrder
        },
        QtObject {
            property string display: i18n("Buzz")
            property string role: "ratingPoints"
            property variant sorting: Qt.DescendingOrder
        },
        QtObject {
            property string display: i18n("Popularity")
            property string role: "popcon"
            property variant sorting: Qt.DescendingOrder
        },
        QtObject {
            property string display: i18n("Origin")
            property string role: "origin"
            property variant sorting: Qt.DescendingOrder
        }//,
//         QtObject {
//             property string display: i18n("Usage")
//             property string role: "usageCount"
//             property variant sorting: Qt.DescendingOrder
//         }
    ]
    
    Component {
        id: categoryHeaderComponent
        CategoryHeader {
            id: categoryHeader
            category: page.category
            height: 100
            width: parent.width
        }
    }
    
    ApplicationsList {
        id: apps
        anchors.fill: parent
        stack: page.pageStack
        header: parent.category==null ? null : categoryHeaderComponent
        visible: !listViewShown.checked
    }
    
    ApplicationsGrid {
        anchors.fill: parent
        stack: page.pageStack
        header: parent.category==null ? null : categoryHeaderComponent
        visible: !apps.visible
        
        category: apps.category
        sortRole: apps.sortRole
        sortOrder: apps.sortOrder
        stateFilter: apps.stateFilter
//         section: apps.section
    }
}