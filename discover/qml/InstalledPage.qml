import QtQuick 2.1
import QtQuick.Controls 1.1

ApplicationsListPage {
    id: page
    stateFilter: 2
    preferList: true
    
    Component {
        id: updatesPage
        UpdatesPage {}
    }
    
    extendedToolBar: Component {
        id: toolbarComponent
        ToolButton {
            id: commitButton
            text: i18n("Update All")
            iconSource: "system-software-update"
            width: resourcesModel.updatesCount>0 ? commitButton.implicitWidth : 0

            onClicked: {
                var page = Stack.view.push(updatesPage)
                page.start()
            }
        }
    }
    
    Component.onCompleted: {
        page.changeSorting("canUpgrade", Qt.AscendingOrder, "canUpgrade")
    }
}
