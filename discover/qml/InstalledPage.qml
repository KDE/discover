import QtQuick 1.1
import org.kde.plasma.components 0.1

ApplicationsListPage {
    id: page
    stateFilter: (1<<8)
    sortRole: "canUpgrade"
    sortOrder: 1
    sectionProperty: "canUpgrade"
    sectionDelegate: Label { text: section=="true" ? i18n("Update") : i18n("Installed"); anchors.right: parent.right }
    preferUpgrade: true
    preferList: true
    
    UpdatesPage {
        id: updatesPage
    }
    
    Component {
        id: toolbarComponent
        ToolButton {
            id: commitButton
            text: i18n("Upgrade All!")
            iconSource: "system-software-update"
            width: app.appBackend.updatesCount>0 && page.state!="updating" ? commitButton.implicitWidth : 0
            
            onClicked: {
                updatesPage.start();
                pageStack.push(updatesPage)
            }
        }
    }
    
    property Item __tools: null
    Component.onCompleted: {
        __tools=toolbarComponent.createObject(page.tools)
    }
    Component.onDestruction: __tools.destroy()
}