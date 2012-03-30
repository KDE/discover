import QtQuick 1.0
import org.kde.plasma.components 0.1
import org.kde.qtextracomponents 0.1
import "navigation.js" as Navigation

Page {
    id: page
    property alias category: appsGrid.category
    property alias sortRole: appsGrid.sortRole
    property alias sortOrder: appsGrid.sortOrder
    property alias stateFilter: appsGrid.stateFilter
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
        visible: !listViewShown.checked
        
        header: appsGrid.header
        category: appsGrid.category
        sortRole: appsGrid.sortRole
        sortOrder: appsGrid.sortOrder
        stateFilter: appsGrid.stateFilter
    }
    
    ApplicationsGrid {
        id: appsGrid
        anchors.fill: parent
        stack: page.pageStack
        visible: !apps.visible
        header: parent.category==null ? null : categoryHeaderComponent
        
//         section: apps.section
        delegate: visible ? flexibleDelegate : null
//         delegate: Rectangle { color: "blue"; width: appsGrid.cellWidth-3; height: appsGrid.cellHeight-3 }
        
        property string delegateType: ""
    }
    
    Component {
        id: flexibleDelegate
        
        Item {
            width: appsGrid.cellWidth-5
            height: appsGrid.cellHeight-5
            property real contHeight: appsGrid.cellHeight*0.7
            
            Item {
                anchors.fill: parent
                visible: appsGrid.delegateType=="icon"
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
            }
            Item {
                visible: appsGrid.delegateType=="screenshot"
                anchors.fill: parent
                
                Image {
                    id: screen
                    anchors {
                        horizontalCenter: parent.horizontalCenter
                        top: parent.top
                        topMargin: 5
                    }
                    fillMode: Image.PreserveAspectFit
                    source: model.application.screenshotUrl(0)
                    width: parent.width; height: contHeight
                    smooth: true
                    asynchronous: true
                    onStatusChanged:  {
                        if(status==Image.Error) {
                            sourceSize.width = height
                            sourceSize.height = height
                            source="image://icon/"+model.application.icon
                            smallIcon.visible=false
                        }
                    }
                }
                Image {
                    id: smallIcon
                    anchors {
                        bottom: screen.bottom
                        right: screen.right
                    }
                    width: 48
                    height: width
                    fillMode: Image.PreserveAspectFit
                    source: "image://icon/"+model.application.icon
                }
                Label {
                    anchors {
                        bottom: parent.bottom
                        left: parent.left
                        right: parent.right
                        leftMargin: 5
                    }
                    font.pointSize: 13
                    horizontalAlignment: Text.AlignHCenter
                    elide: Text.ElideRight
                    text: name
                }
            }
            MouseArea {
                id: delegateArea
                anchors.fill: parent
                onClicked: Navigation.openApplication(stack, application)
                hoverEnabled: true
            }
        }
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
        }
    ]
}