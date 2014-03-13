import QtQuick 2.1
import org.kde.plasma.components 2.0

ApplicationsListPage {
    id: page
    stateFilter: 2
    preferList: true
    
    Component {
        id: updatesPage
        UpdatesPage {}
    }
    
    Component {
        id: toolbarComponent
        ToolButton {
            id: commitButton
            text: i18n("Update All")
            iconSource: "system-software-update"
            width: resourcesModel.updatesCount>0 ? commitButton.implicitWidth : 0
            
            onClicked: {
                var page = pageStack.push(updatesPage)
                page.start()
            }
        }
    }
    
    Component.onCompleted: {
        toolbarComponent.createObject(page.tools)
        page.changeSorting("canUpgrade", Qt.AscendingOrder, "canUpgrade")
    }
}
