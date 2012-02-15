import QtQuick 1.1
import org.kde.plasma.components 0.1
import org.kde.qtextracomponents 0.1
import org.kde.muon 1.0

ListView
{
    property QtObject category
    property alias sortRole: apps.sortRole

    function searchFor(text) {
        apps.search(text)
    }
    Component {
        id: delegate
        ListItem {
            Row {
                spacing: 10
                QIconItem { icon: model["icon"]; width: 40; height: 40 }
                Label { text: name }
            }
            Rating {
                anchors.right: parent.right
                rating: model["rating"]
            }
            
            MouseArea {
                anchors.fill: parent
                onClicked: openApplication(application)
            }
        }
    }
    delegate: delegate
    
    model: ApplicationProxyModel {
        id: apps
        sortRole: 37
        dynamicSortFilter: true
        
        Component.onCompleted: {
            if(category)
                setFiltersFromCategory(category)
            sortModel(0, 1)
        }
    }
}