import QtQuick 1.0
import org.kde.plasma.components 0.1

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
        }
    }
    
    ListView {
        anchors {
            margins: 10
            top: pageToolBar.bottom
            left: parent.left
            right: parent.right
            bottom: parent.bottom
        }
        
        model: app.backend.originLabels()
        
        delegate: ListItem {
            Label { anchors.fill: parent; text: i18n("%1 - %2", modelData, app.backend.origin(modelData)) }
            ToolButton { anchors.right: parent.right; iconSource: "list-remove" }
        }
    }
}
