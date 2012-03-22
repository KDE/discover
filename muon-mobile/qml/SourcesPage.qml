import QtQuick 1.0
import org.kde.plasma.components 0.1
import org.kde.muon 1.0

Item {
    id: page
    
    ToolBar {
        id: pageToolBar
        height: 30
        anchors {
            leftMargin: 10
            rightMargin: 10
            top: parent.top
            right: parent.right
            left: parent.left
        }
        
        tools: Row {
            anchors.fill: parent
            ToolButton {
                iconSource: "list-add"
                text: i18n("Add Source")
                onClicked: newSourceDialog.open()
                anchors.verticalCenter: parent.verticalCenter
            }
            
            Repeater {
                model: ["software_properties"]
                
                delegate: MuonToolButton {
                    property QtObject action: app.getAction(modelData)
                    height: parent.height
                    text: action.text
                    onClicked: action.trigger()
                    enabled: action.enabled
                    icon: action.icon
                }
            }
        }
    }
    
    OriginsBackend {
        id: origins
    }
    
    CommonDialog {
        id: newSourceDialog
        onClickedOutside: reviewDialog.close()
        titleText: i18n("Specify the new source")
        buttons: Row {
            spacing: 5
            Button {
                text: i18n("Submit")
                enabled: repository.text!=""
                onClicked: newSourceDialog.accept()
            }
            Button {
                text: i18n("Cancel")
                onClicked: newSourceDialog.reject()
            }
        }
        
        content: Item {
            height: 200
            width: 500
            
            Column {
                id: info
                spacing: 5
                anchors.fill: parent
                
                Label {
                    anchors {
                        left: parent.left
                        right: parent.right
                        margins: 10
                    }
                    wrapMode: Text.WordWrap
                    text: i18n("<sourceline> - The apt repository source line to add. This is one of:\n"
                                +"  a complete apt line, \n"
                                +"  a repo url and areas (areas defaults to 'main')\n"
                                +"  a PPA shortcut.\n\n"

                                +"  Examples:\n"
                                +"    deb http://myserver/path/to/repo stable myrepo\n"
                                +"    http://myserver/path/to/repo myrepo\n"
                                +"    https://packages.medibuntu.org free non-free\n"
                                +"    http://extras.ubuntu.com/ubuntu\n"
                                +"    ppa:user/repository") }
                TextField {
                    id: repository
                    anchors.left: parent.left
                    anchors.right: parent.right
//                     onAccepted: newSourceDialog.accept()
                }
            }
        }
        
        onAccepted: origins.addRepository(repository.text)
    }
    
    ListView {
        anchors {
            margins: 10
            top: pageToolBar.bottom
            left: parent.left
            right: parent.right
            bottom: parent.bottom
        }
        clip: true
        
        model: origins.labels
        
        delegate: ListItem {
            Label {
                anchors {
                    fill: parent
                    leftMargin: removeButton.width+5
                }
                text: i18n("%1 - %2", modelData, origins.labelsOrigin(modelData))
            }
            ToolButton { id: removeButton; anchors.left: parent.left; iconSource: "list-remove" }
        }
    }
}
