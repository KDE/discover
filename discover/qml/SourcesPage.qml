import QtQuick 1.0
import org.kde.plasma.components 0.1
import org.kde.muon 1.0
import org.kde.muonapt 1.0
import "navigation.js" as Navigation

Page {
    id: page
    clip: true
    property real actualWidth: width-Math.pow(width/70, 2)
    
    tools: Row {
        anchors.fill: parent
        visible: page.visible
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
    
    
    CommonDialog {
        id: newSourceDialog
        onClickedOutside: reviewDialog.close()
        titleText: i18n("Specify the new source")
        buttons: Row {
            spacing: 5
            Button {
                text: i18n("OK")
                iconSource: "dialog-ok"
                enabled: repository.text!=""
                onClicked: newSourceDialog.accept()
            }
            Button {
                text: i18n("Cancel")
                iconSource: "dialog-cancel"
                onClicked: newSourceDialog.reject()
            }
        }
        
        content: Item {
            height: info.childrenRect.height
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
                    Keys.onEnterPressed: newSourceDialog.accept()
                    focus: true
                }
            }
        }
        
        onAccepted: origins.addRepository(repository.text)
    }
    OriginsBackend { id: origins }
    
    NativeScrollBar {
        id: scroll
        orientation: Qt.Vertical
        flickableItem: view
        anchors {
            top: view.top
            right: parent.right
            bottom: view.bottom
        }
    }
    ListView {
        id: view
        anchors {
            top: parent.top
            bottom: parent.bottom
            horizontalCenter: parent.horizontalCenter
        }
        width: parent.actualWidth
        
        model: origins.sources
        
        delegate: ListItem {
            function joinEntriesSuites(source) {
                var vals = {}
                for(var i=0; i<source.entries.length; ++i) {
                    var entry = source.entries[i]
                    if(vals[entry.suite]==null)
                        vals[entry.suite]=0
                    
                    if(entry.isSource)
                        vals[entry.suite] += 2
                    else
                        vals[entry.suite] += 1
                }
                var ret = new Array
                for(var e in vals) {
                    if(vals[e]>1)
                        ret.push(e)
                    else
                        ret.push(i18n("%1 (Binary)", e))
                }
                
                return ret.join(", ")
            }
            enabled: browseOrigin.enabled
            onClicked: Navigation.openApplicationListSource(modelData.name)
            
            CheckBox {
                id: enabledBox
                enabled: false //TODO: implement the application of this change
                anchors {
                    left: parent.left
                    top: parent.top
                }
                checked: modelData.enabled
            }
            Label {
                anchors {
                    top: parent.top
                    bottom: parent.bottom
                    left: enabledBox.right
                    right: suitesLabel.left
                    leftMargin: 5
                }
                elide: Text.ElideRight
                text: modelData.name=="" ? modelData.uri : i18n("%1. %2", modelData.name, modelData.uri)
            }
            Label {
                id: suitesLabel
                anchors {
                    bottom: parent.bottom
                    right: browseOrigin.left
                }
                text: joinEntriesSuites(modelData)
            }
            ToolButton {
                id: browseOrigin
                enabled: modelData.name!=""
                iconSource: "view-filter"
                onClicked: Navigation.openApplicationListSource(modelData.name)
                anchors {
                    bottom: parent.bottom
                    right: removeButton.left
                }
                
            }
            ToolButton {
                id: removeButton
                anchors.right: parent.right
                iconSource: "edit-delete"
                onClicked: origins.removeRepository(modelData.uri)
            }
        }
    }
}
