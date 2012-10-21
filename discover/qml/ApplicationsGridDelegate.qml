import org.kde.plasma.components 0.1
import org.kde.qtextracomponents 0.1
import "navigation.js" as Navigation
import QtQuick 1.1

ListItem {
    id: delegateRoot
    clip: true
    width: parentItem.cellWidth
    height: parentItem.cellHeight
    property bool requireClick: false
    
    MouseArea {
        id: delegateArea
        anchors.fill: parent
        hoverEnabled: true
        property bool displayDescription: false
        onPositionChanged: if(!delegateRoot.requireClick) timer.restart()
        onExited: { if(!delegateRoot.requireClick) timer.stop(); displayDescription=false}
        Timer {
            id: timer
            interval: 200
            onTriggered: delegateArea.displayDescription=true
        }
        onClicked: {
            if(delegateRoot.requireClick && !displayDescription) {
                displayDescription=true
            } else
                Navigation.openApplication(application)
        }
    
        Flickable {
            id: delegateFlickable
            anchors.fill: parent
            contentHeight: delegateRoot.height*2
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
                source: model.application.thumbnailUrl
                width: parent.width; height: delegateRoot.height*0.7
                sourceSize {
                    width: screen.width
                    height: screen.height
                }
                cache: false
                asynchronous: true
                onStatusChanged:  {
                    if(status==Image.Error) {
                        fallbackToIcon()
                    }
                }
                Component.onCompleted: {
                    if(model.application.thumbnailUrl=="")
                        fallbackToIcon();
                }
                
                function fallbackToIcon() {
                    sourceSize.width = height
                    sourceSize.height = height
                    source="image://icon/"+model.application.icon
                    smallIcon.visible = false
                }
            }
            QIconItem {
                id: smallIcon
                anchors {
                    right: screen.right
                }
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
                horizontalAlignment: Text.AlignHCenter
                elide: Text.ElideRight
                text: name
            }
            Loader {
                id: descriptionLoader
                anchors {
                    left: parent.left
                    right: parent.right
                    bottom: parent.bottom
                    top: parent.verticalCenter
                }
            }
        }
        onStateChanged: {
            if(state=="description")
                descriptionLoader.sourceComponent=extraInfoComponent
        }
        
        state: "screenshot"
        states: [
            State { name: "screenshot"
                when: !delegateArea.displayDescription
                PropertyChanges { target: smallIcon; y: 5+screen.height-height }
                PropertyChanges { target: delegateFlickable; contentY: 0 }
            },
            State { name: "description"
                when: delegateArea.displayDescription
                PropertyChanges { target: smallIcon; y: 5+delegateRoot.height }
                PropertyChanges { target: delegateFlickable; contentY: delegateRoot.height }
            }
        ]
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
                }
                height: installButton.height
                Rating {
                    anchors.centerIn: parent
                    height: parent.height*0.7
                    rating: model.rating
                    visible: !model.application.canUpgrade && model.rating>=0
                }
                Button {
                    anchors.fill: parent
                    text: i18n("Update")
                    visible: model.application.canUpgrade
                }
            }
        }
    }
}
