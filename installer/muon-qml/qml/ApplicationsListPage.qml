import org.kde.plasma.components 0.1

Page {
    property alias category: apps.category
    property alias sortRole: apps.sortRole
    property alias stateFilter: apps.stateFilter
    
    function searchFor(text) {
        apps.searchFor(text)
    }
    
    ApplicationsList {
        id: apps
        anchors.fill: parent
    }
}