import QtQuick 1.0
import org.kde.plasma.components 0.1
import org.kde.muon 1.0

Item {
    id: page
    
    ToolBar {
        id: pageToolBar
        height: 30
        anchors {
            leftMargin: 10
            rightMargin: 10
            top: parent.top
            right: parent.right
            left: parent.left
        }
        
        tools: Row {
            anchors.fill: parent
            ToolButton {
                iconSource: "list-add"
                text: i18n("Add Source")
                anchors.verticalCenter: parent.verticalCenter
            }
            
            Repeater {
                model: ["software_properties"]
                
                delegate: MuonToolButton {
                    property QtObject action: app.getAction(modelData)
                    height: parent.height
                    text: action.text
                    onClicked: action.trigger()
                    enabled: action.enabled
                    icon: action.icon
                }
            }
        }
    }
    
    OriginsBackend {
        id: origins
    }
    
    ListView {
        anchors {
            margins: 10
            top: pageToolBar.bottom
            left: parent.left
            right: parent.right
            bottom: parent.bottom
        }
        clip: true
        
        model: origins.labels
        
        delegate: ListItem {
            Label { anchors.fill: parent; anchors.leftMargin: removeButton.width; text: i18n("%1 - %2", modelData, origins.labelsOrigin(modelData)) }
            ToolButton { id: removeButton; anchors.left: parent.left; iconSource: "list-remove" }
        }
    }
}
