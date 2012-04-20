import QtQuick 1.1
import org.kde.plasma.components 0.1
import org.kde.qtextracomponents 0.1
import "navigation.js" as Navigation
import org.kde.muon 1.0

Page {
    id: page
    property alias category: appsModel.filteredCategory
    property alias sortRole: appsModel.stringSortRole
    property alias sortOrder: appsModel.sortOrder
    property alias stateFilter: appsModel.stateFilter
    property alias originFilter: appsModel.originFilter
    property alias originHostFilter: appsModel.originHostFilter //hack to be able to provide the url
    property alias section: apps.section
    property bool preferUpgrade: false
    clip: true
    
    function useList() { state="list" }
    
    function searchFor(text) {
        appsModel.search(text)
        state="list"
    }
    
    ApplicationProxyModel {
        id: appsModel
        stringSortRole: "ratingPoints"
        sortOrder: Qt.DescendingOrder
        
        Component.onCompleted: sortModel()
    }
    
    tools: Row {
            id: buttonsRow
            width: 100
            visible: page.visible
            MuonMenuToolButton {
                id: button
                icon: "view-sort-ascending"
                anchors.verticalCenter: parent.verticalCenter
                model: paramModel
                delegate: ToolButton {
                    width: parent.width
                    text: display
                    onClicked: {
                        appsModel.stringSortRole=role
                        appsModel.sortOrder=sorting
                        button.checked=false
                    }
                    checked: appsModel.stringSortRole==role
                }
            }
            
            MuonMenuToolButton {
                id: listViewShown
                checkable: true
                icon: "tools-wizard"
                model: ["list", "grid1", "grid2", "grid3"]
                delegate: ToolButton {
                    width: parent.width
                    text: modelData
                    onClicked: {
                        page.state=modelData
                        listViewShown.checked=false
                    }
                    checked: page.state==modelData
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
        visible: !listViewShown.checked
        preferUpgrade: page.preferUpgrade
        
        header: appsGrid.header
        model: appsModel
    }
    
    ApplicationsGrid {
        id: appsGrid
        anchors.fill: parent
        model: appsModel
        visible: !apps.visible
        header: parent.category==null ? null : categoryHeaderComponent
        
        delegate: ApplicationsGridDelegate { id: flexibleDelegate }
        
        property string delegateType: ""
    }
    
    state: "grid2"
    states: [
        State {
            name: "list"
            PropertyChanges { target: apps; visible: true }
        },
        State {
            name: "grid1"
            PropertyChanges { target: apps; visible: false }
            PropertyChanges { target: appsGrid; delegateType: "icon" }
        },
        State {
            name: "grid2"
            PropertyChanges { target: apps; visible: false }
            PropertyChanges { target: appsGrid; delegateType: "screenshot" }
        },
        State {
            name: "grid3"
            PropertyChanges { target: apps; visible: false }
            PropertyChanges { target: appsGrid; delegateType: "still" }
        }
    ]
}