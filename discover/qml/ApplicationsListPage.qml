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
    property alias originFilter: appsModel.originFilter
    property alias mimeTypeFilter: appsModel.mimeTypeFilter
    property alias stateFilter: appsModel.stateFilter
    property bool shouldShowTechnical: category ? category.shouldShowTechnical: false
    property string sectionProperty: ""
    property string search: ""
    property Component sectionDelegate: null
    property bool preferUpgrade: false
    property bool preferList: false
    property real actualWidth: width-Math.pow(width/70, 2)
    property real proposedMargin: (width-actualWidth)/2
    clip: true
    
    onSearchChanged: {
        appsModel.search(search)
        appsModel.sortOrder = Qt.AscendingOrder
    }
    
    function searchFor(text) {
        search = text
    }
    
    ApplicationProxyModel {
        id: appsModel
        stringSortRole: "ratingPoints"
        sortOrder: Qt.DescendingOrder
        isShowingTechnical: page.shouldShowTechnical
        
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
                        page.sectionProperty = section
                        button.checked=false
                    }
                    checked: appsModel.stringSortRole==role
                }
            }
            
            MuonMenuToolButton {
                id: listViewShown
                checkable: true
                icon: "tools-wizard"
                model: ["list", "grid2"]
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
            property string section: ""
            property variant sorting: Qt.AscendingOrder
        },
        QtObject {
            property string display: i18n("Rating")
            property string role: "sortableRating"
            property string section: ""
            property variant sorting: Qt.DescendingOrder
        },
        QtObject {
            property string display: i18n("Buzz")
            property string role: "ratingPoints"
            property string section: ""
            property variant sorting: Qt.DescendingOrder
        },
        QtObject {
            property string display: i18n("Popularity")
            property string role: "popcon"
            property string section: ""
            property variant sorting: Qt.DescendingOrder
        },
        QtObject {
            property string display: i18n("Origin")
            property string role: "origin"
            property string section: "origin"
            property variant sorting: Qt.DescendingOrder
        }
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
    
    Loader {
        id: viewLoader
        anchors.fill: parent
    }
    
    Component {
        id: listComponent
        ApplicationsList {
            id: apps
            anchors.fill: parent
            preferUpgrade: page.preferUpgrade
            section.property: page.sectionProperty
            section.delegate: page.sectionDelegate ? page.sectionDelegate : defaultSectionDelegate
            actualWidth: page.actualWidth
            
            Component {
                id: defaultSectionDelegate
                Label {
                    text: section
                    anchors {
                        right: parent.right
                        rightMargin: apps.proposedMargin
                    }
                }
            }
            
            header: page.category==null ? null : categoryHeaderComponent
            model: appsModel
        }
    }
    
    Component {
        id: gridComponent
        ApplicationsGrid {
            model: appsModel
            header: page.category==null ? null : categoryHeaderComponent
            actualWidth: page.actualWidth
            
            delegate: ApplicationsGridDelegate {}
        }
    }
    
    state: preferList ? "list" : "grid2"
    states: [
        State {
            name: "list"
            PropertyChanges { target: viewLoader; sourceComponent: listComponent }
        },
        State {
            name: "grid2"
            PropertyChanges { target: viewLoader; sourceComponent: gridComponent }
        }
    ]
}