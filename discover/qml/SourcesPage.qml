import QtQuick 2.1
import QtQuick.Controls 1.1
import QtQuick.Layouts 1.1
import org.kde.muon 1.0
import "navigation.js" as Navigation

Item {
    id: page
    clip: true
    property real actualWidth: width-Math.pow(width/70, 2)
    property real proposedMargin: (page.width-actualWidth)/2

    Menu {
        id: sourcesMenu
    }

    property Component tools: RowLayout {
        anchors.fill: parent
        visible: page.visible
        ToolButton {
            iconName: "list-add"
            text: i18n("Add Source")

            menu: sourcesMenu
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
    
    ScrollView {
        anchors.fill: parent
        ListView {
            id: view
            width: parent.width

            model: SourcesModel

            delegate: ColumnLayout {
                x: page.proposedMargin
                width: page.actualWidth

                AddSourceDialog {
                    id: addSourceDialog
                    source: model.sourceBackend

                    onVisibleChanged: if(!visible) destroy()
                }

                MenuItem {
                    id: menuItem
                    text: model.display
                    onTriggered: {
                        try {
                            addSourceDialog.open()
                            addSourceDialog.visible = true
                        } catch (e) {
                            console.log("error loading dialog:", e)
                        }
                    }
                }

                Component.onCompleted: {
                    sourcesMenu.insertItem(0, menuItem)
                }

                Label { text: sourceBackend.name }
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
