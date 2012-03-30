import QtQuick 1.0
import org.kde.plasma.components 0.1
import org.kde.qtextracomponents 0.1

Page {
    id: page
    property alias category: apps.category
    property alias sortRole: apps.sortRole
    property alias sortOrder: apps.sortOrder
    property alias stateFilter: apps.stateFilter
    property alias section: apps.section
    
    function searchFor(text) {
        apps.searchFor(text)
    }
    
    tools: Row {
            id: buttonsRow
            width: 100
            visible: page.status == PageStatus.Active
            MuonMenuToolButton {
                id: button
                icon: "view-sort-ascending"
                anchors.verticalCenter: parent.verticalCenter
                model: paramModel
                delegate: ToolButton {
                    width: parent.width
                    text: display
                    onClicked: {
                        apps.sortRole=role
                        apps.sortOrder=sorting
                    }
                    checked: apps.sortRole==role
                }
            }
            
            MuonMenuToolButton {
                id: listViewShown
                checkable: true
                icon: "tools-wizard"
                model: ["list", "grid1", "grid2"]
                delegate: ToolButton {
                    width: parent.width
                    text: modelData
                    onClicked: page.state=modelData
                    checked: apps.state==modelData
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
    
    states: [
        State {
            name: "list"
            PropertyChanges { target: apps; visible: true }
        },
        State {
            name: "grid1"
            PropertyChanges { target: apps; visible: false }
        },
        State {
            name: "grid2"
            PropertyChanges { target: apps; visible: false }
        }
    ]
}