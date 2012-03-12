import org.kde.plasma.components 0.1

Page {
    id: page
    property alias category: apps.category
    property alias sortRole: apps.sortRole
    property alias stateFilter: apps.stateFilter
    
    tools: TextField {
        width: 80
        placeholderText: i18n("Search...")
        onTextChanged: apps.searchFor(text)
        opacity: page.status == PageStatus.Active ? 1 : 0
    }
    
    ApplicationsList {
        id: apps
        anchors.fill: parent
    }
}