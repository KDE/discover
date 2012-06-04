import QtQuick 1.1
import org.kde.plasma.components 0.1

ApplicationsListPage {
    id: page
//     stateFilter: (1<<8)
    sortRole: "canUpgrade"
    sortOrder: 1
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
    
    UpdatesPage {
        id: updatesPage
    }
    
    Component {
        id: toolbarComponent
        ToolButton {
            id: commitButton
            text: i18n("Update All!")
            iconSource: "system-software-update"
//             width: app.appBackend.updatesCount>0 ? commitButton.implicitWidth : 0
            
            onClicked: {
                updatesPage.start();
                pageStack.push(updatesPage)
            }
        }
    }
    
    Component.onCompleted: {
        toolbarComponent.createObject(page.tools)
    }
}