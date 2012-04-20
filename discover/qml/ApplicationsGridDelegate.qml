import org.kde.plasma.components 0.1
import QtQuick 1.1

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
        onClicked: Navigation.openApplication(application)
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
            contentY: delegateArea.containsMousePermanent && appsGrid.delegateType=="screenshot" ? contentHeight/2 : 0
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
                cache: false
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
                font.pointSize: descLabel.font.pointSize*1.2
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
                preferUpgrade: page.preferUpgrade
            }
            Item {
                anchors {
                    right: parent.right
                    left: installButton.right
                    verticalCenter: installButton.verticalCenter
                    margins: 10
                }
                height: Math.min(installButton.height, width/5)
                Rating {
                    rating: model.rating
                    visible: !model.application.canUpgrade
                }
                Button {
                    text: i18n("Upgrade")
                    visible: model.application.canUpgrade
                }
            }
        }
    }
}