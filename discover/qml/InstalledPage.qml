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
        Button {
            id: commitButton
            text: i18n("Update All")
            iconSource: "system-software-update"
            width: ResourcesModel.updatesCount>0 ? commitButton.implicitWidth : 0

            onClicked: {
                var updates = page.Stack.view.push(updatesPage)
                updates.start()
            }
        }
    }
    
    Component.onCompleted: {
        page.changeSorting("canUpgrade", Qt.AscendingOrder, "canUpgrade")
    }
}
