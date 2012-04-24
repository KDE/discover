import org.kde.plasma.components 0.1
import org.kde.qtextracomponents 0.1
import "navigation.js" as Navigation
import QtQuick 1.1

ListItem {
    id: delegateRoot
    clip: true
    width: parentItem.cellWidth
    height: parentItem.cellHeight
    
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
            contentHeight: delegateRoot.height*2
            contentY: delegateArea.containsMousePermanent ? delegateRoot.height : 0
            interactive: false
            Behavior on contentY { NumberAnimation { duration: 200; easing.type: Easing.InQuad } }
            
            Image {
                id: screen
                anchors {
                    horizontalCenter: parent.horizontalCenter
                    top: parent.top
                    topMargin: 5
                }
                fillMode: Image.PreserveAspectFit
                source: model.application.screenshotUrl(0)
                width: parent.width; height: delegateRoot.height*0.7
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
            QIconItem {
                id: smallIcon
                anchors {
                    right: screen.right
                }
                y: 5+(delegateArea.containsMousePermanent ? delegateRoot.height : (screen.height-height))
                width: 48
                height: width
                icon: model.application.icon
                Behavior on y { NumberAnimation { duration: 200; easing.type: Easing.InQuad } }
            }
            Label {
                anchors {
                    top: screen.bottom
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
                    bottom: parent.bottom
                    top: parent.verticalCenter
                }
                sourceComponent: delegateArea.containsMousePermanent ? extraInfoComponent : null
            }
        }
    }
    
    Component {
        id: extraInfoComponent
        Item {
            Label {
                anchors {
                    left: parent.left
                    right: parent.right
                    top: parent.top
                    bottom: installButton.top
                    rightMargin: smallIcon.visible ? 48 : 0
                    topMargin: 5
                }
                verticalAlignment: Text.AlignVCenter
                horizontalAlignment: Text.AlignHCenter
                width: parent.width-48
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
                    anchors.fill: parent
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