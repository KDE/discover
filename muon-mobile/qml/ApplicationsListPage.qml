import QtQuick 1.0
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
    property alias section: apps.section
    clip: true
    
    function searchFor(text) {
        appsModel.search(text)
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
                        appsModel.stringSortRole=role
                        appsModel.sortOrder=sorting
                    }
                    checked: appsModel.stringSortRole==role
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
        stack: page.pageStack
        visible: !listViewShown.checked
        
        header: appsGrid.header
        model: appsModel
    }
    
    ApplicationsGrid {
        id: appsGrid
        anchors.fill: parent
        model: appsModel
        visible: !apps.visible
        header: parent.category==null ? null : categoryHeaderComponent
        
        delegate: visible ? flexibleDelegate : null
        
        property string delegateType: ""
    }
    
    Component {
        id: flexibleDelegate
        
        ListItem {
            clip: true
            property real contHeight: appsGrid.cellHeight*0.7
            property bool enlarge: appsGrid.delegateType=="icon" && delegateArea.containsMousePermanent
            width: appsGrid.cellWidth-5
            height: !enlarge ? appsGrid.cellHeight-5 : delegateFlickable.contentHeight
            Behavior on height { NumberAnimation { duration: 200 } }
            
            Rectangle {
                color: "white"; anchors.fill: parent; opacity: enlarge ? 0.9 : 0
                Behavior on opacity { NumberAnimation { duration: 200 } }
            }
            z: enlarge ? 123123 : -123123
            
            MouseArea {
                id: delegateArea
                anchors.fill: parent
                onClicked: Navigation.openApplication(page.pageStack, application)
                hoverEnabled: true
                property bool containsMousePermanent: false
                onPositionChanged: timer.restart()
                onExited: { timer.stop(); containsMousePermanent=false}
                Timer {
                    id: timer
                    interval: 200
                    onTriggered: delegateArea.containsMousePermanent=true
                }
            
                Flickable {
                    id: delegateFlickable
                    width: parent.width
                    height: parent.height
                    contentHeight: appsGrid.delegateType=="icon" ? (appsGrid.cellHeight+descLabel.height+installButton.height+10) : (appsGrid.cellHeight*2-10)
                    contentY: delegateArea.containsMousePermanent && appsGrid.delegateType!="icon" ? contentHeight/2 : 0
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
                            right: parent.right
                        }
                        y: appsGrid.cellHeight+10
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
                        y:  appsGrid.delegateType=="icon" ? installButton.y-height : appsGrid.cellHeight+smallIconDesc.height/2
                        wrapMode: Text.WordWrap
                        text: model.application.comment
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