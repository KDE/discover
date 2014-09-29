import QtQuick 2.1
import QtQuick.Controls 1.1
import QtQuick.Dialogs 1.2
import QtQuick.Layouts 1.1
import org.kde.muon 1.0
import "navigation.js" as Navigation

Item {
    id: page
    clip: true
    property real actualWidth: width-Math.pow(width/70, 2)
    property real proposedMargin: (page.width-actualWidth)/2
    
    property Component tools: Row {
        anchors.fill: parent
        visible: page.visible
        ToolButton {
            iconName: "list-add"
            text: i18n("Add Source")
            onClicked: newSourceDialog.open()
            anchors.verticalCenter: parent.verticalCenter
        }
        
//         Repeater {
//             model: ["software_properties"]
//
//             delegate: MuonToolButton {
//                 property QtObject action: app.getAction(modelData)
//                 height: parent.height
//                 text: action.text
//                 onClicked: action.trigger()
//                 enabled: action.enabled
// //                 icon: action.icon
//             }
//         }
    }
    
    Dialog {
        id: newSourceDialog
        title: i18n("Specify the new source")
        standardButtons: StandardButton.Ok | StandardButton.Close
        
        Item {
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
    
    ScrollView {
        anchors.fill: parent
        ListView {
            id: view
            width: parent.width

            model: SourcesModel

            delegate: ColumnLayout {
                x: page.proposedMargin
                width: page.actualWidth
                Label { text: "Shit" }
                Repeater {
                    model: sourceBackend.sources

                    delegate: GridItem {
                        width: parent.width
                        height: browseOrigin.height*1.2
                        enabled: browseOrigin.enabled
                        onClicked: Navigation.openApplicationListSource(model.display)

                        RowLayout {
                            Layout.alignment: Qt.AlignVCenter
                            anchors.centerIn: parent
                            width: parent.width

                            CheckBox {
                                id: enabledBox
                                enabled: false //TODO: implement the application of this change
                                checked: model.checked != Qt.Unchecked
                            }
                            Label {
                                Layout.fillWidth: true
                                elide: Text.ElideRight
                                text: model.display
                            }
                            Label {
                                text: model.toolTip
                            }
                            Button {
                                id: browseOrigin
                                enabled: display!=""
                                iconName: "view-filter"
                                onClicked: Navigation.openApplicationListSource(model.name)
                            }
                            Button {
                                iconName: "edit-delete"
                                onClicked: origins.removeRepository(model.uri)
                            }
                        }
                    }
                }
            }
        }
    }
}
