import QtQuick 1.0
import org.kde.plasma.components 0.1

Item {
    
    ListView {
        anchors.fill: parent
        model: app.backend.originLabels()
        
        delegate: ListItem {
            
            Label { anchors.fill: parent; text: i18n("%1 - %2", modelData, app.backend.origin(modelData)) }
        }
    }
}