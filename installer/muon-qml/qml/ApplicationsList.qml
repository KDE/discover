import QtQuick 1.1
import org.kde.plasma.components 0.1
import org.kde.qtextracomponents 0.1
import org.kde.muon 1.0

Page {
    property variant category
    
    Component {
        id: delegate
        ListItem {
            Row {
                spacing: 10
                QIconItem { icon: model["icon"]; width: 40; height: 40 }
                Label { text: name }
            }
            
            MouseArea {
                anchors.fill: parent
                onClicked: openApplication(application)
            }
        }
    }
    
    ListView
    {
        anchors.fill: parent
        delegate: delegate
        
        model: ApplicationProxyModel {
            
            Component.onCompleted: {
                setFiltersFromCategory(category)
            }
        }
    }
}