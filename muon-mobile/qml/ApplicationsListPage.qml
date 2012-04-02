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
    clip: true
    
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
        
        ListItem {
            clip: true
            property real contHeight: appsGrid.cellHeight*0.7
            property bool enlarge: appsGrid.delegateType=="icon" && delegateArea.containsMouse
            width: appsGrid.cellWidth-5
            height: !enlarge ? appsGrid.cellHeight-5 : delegateFlickable.contentHeight
            
            Rectangle { color: "white"; visible: enlarge; anchors.fill: parent; opacity: 0.9 }
            z: enlarge ? 123123 : -123123
            
            MouseArea {
                id: delegateArea
                anchors.fill: parent
                onClicked: Navigation.openApplication(stack, application)
                hoverEnabled: true
            
                Flickable {
                    id: delegateFlickable
                    width: parent.width
                    height: parent.height
                    contentHeight: appsGrid.delegateType=="icon" ? (appsGrid.cellHeight+descLabel.height+installButton.height) : (appsGrid.cellHeight*2-10)
                    contentY: delegateArea.containsMouse && appsGrid.delegateType!="icon" ? contentHeight/2 : 0
                    interactive: false
                    Behavior on contentY { NumberAnimation { duration: 200 } }
                    
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
                                smallIcon.visible = false
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
                            top: smallIcon.bottom
                            left: parent.left
                            right: parent.right
                            leftMargin: 5
                        }
                        font.pointSize: 13
                        horizontalAlignment: Text.AlignHCenter
                        elide: Text.ElideRight
                        text: name
                    }
                    Image {
                        id: smallIconDesc
                        anchors {
                            top: descLabel.top
                            right: parent.right
                        }
                        height: width
                        fillMode: Image.PreserveAspectFit
                        source: "image://icon/"+model.application.icon
                        width: appsGrid.delegateType=="icon" ? 0 : 48
                    }
                    Label {
                        id: descLabel
                        anchors {
                            left: parent.left
                            right: smallIconDesc.left
                            topMargin: 5
                        }
                        horizontalAlignment: Text.AlignHCenter
                        width: parent.width
                        y:  appsGrid.delegateType=="icon" ? installButton.y-height : appsGrid.cellHeight
                        wrapMode: Text.WordWrap
                        text: model.application.comment
                        visible: delegateArea.containsMouse
                    }
                    InstallApplicationButton {
                        id: installButton
                        width: parent.width/3
                        height: 30
                        anchors {
                            bottom: parent.bottom
                            left: parent.left
                            bottomMargin: 20
                            margins: 10
                        }
                        
                        application: model.application
                        preferUpgrade: false //TODO: review
                    }
                    Rating {
                        id: ratingsItem
                        anchors {
                            right: parent.right
                            verticalCenter: installButton.verticalCenter
                            margins: 10
                        }
                        height: installButton.height*0.7
                        rating: model.rating
                    }
                }
            }
        }
    }
    
    state: "grid1"
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