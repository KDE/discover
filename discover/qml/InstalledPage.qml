import QtQuick 2.1
import QtQuick.Controls 1.1
import QtQuick.Layouts 1.1
import org.kde.muon 1.0

ApplicationsListPage {
    id: page
    stateFilter: 2
    preferList: true
    
    Component {
        id: updatesPage
        UpdateProgressPage {}
    }
    
    extendedToolBar: Component {
        id: toolbarComponent
        RowLayout {
            Button {
                id: commitButton
                text: i18n("Update All")
                iconSource: "system-software-update"
                visible: ResourcesModel.updatesCount>0
                width: ResourcesModel.updatesCount>0 ? commitButton.implicitWidth : 0

                onClicked: {
                    var updates = page.Stack.view.push(updatesPage)
                    updates.start()
                }
            }
        }
    }
    
    Component.onCompleted: {
        page.changeSorting("canUpgrade", Qt.AscendingOrder, "canUpgrade")
    }
}
