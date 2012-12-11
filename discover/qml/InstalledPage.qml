import QtQuick 1.1
import org.kde.plasma.components 0.1

ApplicationsListPage {
    id: page
    stateFilter: 2
    sortRole: "canUpgrade"
    sortOrder: Qt.AscendingOrder
    sectionProperty: "canUpgrade"
    sectionDelegate: Label {
        text: section=="true" ? i18n("Update") : i18n("Installed")
        anchors {
            right: parent.right
            rightMargin: page.proposedMargin
        }
    }
    preferUpgrade: true
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
    }
}
