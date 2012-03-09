import QtQuick 1.1
import org.kde.plasma.components 0.1
import org.kde.muon 1.0

Item {
    id: page
    property QtObject backend: app.appBackend
    property bool active: installedView.count>0 || installingView.count>0
    width: 300
    height: contents.height+2*contents.anchors.margins
    
    Rectangle {
        id: bg
        anchors.fill: parent
        color: "blue"
        opacity: 0.2
        radius: 10
    }
    
    onVisibleChanged: opacity=(visible? 1 : 0)
    
    Behavior on opacity {
        NumberAnimation { duration: 250; easing.type: Easing.InOutQuad }
    }
    
    Column {
        id: contents
        anchors {
            left: parent.left
            right: parent.right
            top: parent.top
            margins: 10
        }
        add: Transition {
            NumberAnimation {
                properties: "y"
                easing.type: Easing.OutBounce
            }
        }
        
        MuonToolButton {
            visible: installedView.count>0
            anchors.right: parent.right
            icon: "dialog-close"
            onClicked: backend.clearLaunchList()
        }
        
        Label {
            visible: installedView.count>0
            text: i18n("Installed:")
        }
        
        Repeater {
            id: installedView
            model: LaunchListModel {
                id: launchModel
                backend: page.backend
            }
            
            delegate: ListItem {
                Label {
                    anchors.fill: parent
                    text: display
                }
                MuonToolButton {
                    icon: decoration
                    anchors.right: parent.right
                    anchors.verticalCenter: parent.verticalCenter
                    onClicked: launchModel.invokeApplication(index)
                }
            }
        }
        
        Label {
            text: i18n("Installing:")
            visible: installingView.count>0
        }
        
        Repeater {
            id: installingView
            model: TransactionsModel {
                backend: page.backend
            }
            delegate: ListItem {
                Label {
                    anchors.fill: parent
                    text: display
                }
            }
        }
    }
}
