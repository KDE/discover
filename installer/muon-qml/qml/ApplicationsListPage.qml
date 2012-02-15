import org.kde.plasma.components 0.1

Page {
    property alias category: apps.category
    property alias sortRole: apps.sortRole
    
    function searchFor(text) {
        apps.search(text)
    }
    
    ApplicationsList {
        id: apps
        anchors.fill: parent
    }
}