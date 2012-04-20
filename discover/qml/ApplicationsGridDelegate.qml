import org.kde.plasma.components 0.1
import "navigation.js" as Navigation
import QtQuick 1.1

ListItem {
    clip: true
    width: parentItem.cellWidth-5
    height: parentItem.cellHeight-5
    
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
            contentHeight: delegateArea.height
            contentY: delegateArea.containsMousePermanent ? contentHeight/2 : 0
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
                width: parent.width; height: parent.height*0.7
                cache: false
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
                id: appName
                horizontalAlignment: Text.AlignHCenter
                elide: Text.ElideRight
                text: name
            }
            Loader {
                anchors {
                    left: parent.left
                    right: parent.right
                    top: appName.bottom
                }
                height: delegateFlickable.contentHeight/2
                sourceComponent: delegateArea.containsMousePermanent ? extraInfoComponent : null
            }
        }
    }
    
    Component {
        id: extraInfoComponent
        Item {
            Image {
                id: smallIconDesc
                anchors {
                    right: parent.right
                }
                y: delegateArea.height+5
                height: width
                fillMode: Image.PreserveAspectFit
                source: "image://icon/"+model.application.icon
                width: 48
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
                y: installButton.y-height
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
                    topMargin: 20
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