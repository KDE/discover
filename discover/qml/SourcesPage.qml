import QtQuick 2.1
import QtQuick.Controls 1.1
import QtQuick.Layouts 1.1
import org.kde.muon 1.0
import org.kde.kquickcontrolsaddons 2.0
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
        Repeater {
            model: SourcesModel.actions

            delegate: RowLayout{
                QIconItem {
                    icon: modelData.icon
                }
                ToolButton {
                    property QtObject action: modelData
                    height: parent.height
                    text: action.text
                    onClicked: action.trigger()
                    enabled: action.enabled
                }
            }
        }
    }
    
    ScrollView {
        anchors.fill: parent
        ListView {
            id: view
            width: parent.width

            model: SourcesModel

            delegate: ColumnLayout {
                id: sourceDelegate
                x: page.proposedMargin
                width: page.actualWidth

                property QtObject sourceBackend: model.sourceBackend
                AddSourceDialog {
                    id: addSourceDialog
                    source: sourceDelegate.sourceBackend

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
                        width: sourceDelegate.width
                        height: browseOrigin.height*1.2
                        enabled: browseOrigin.enabled
                        onClicked: Navigation.openApplicationListSource(model.display)

                        RowLayout {
                            Layout.alignment: Qt.AlignVCenter
                            anchors.fill: parent

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
                                onClicked: Navigation.openApplicationListSource(model.display)
                            }
                            Button {
                                iconName: "edit-delete"
                                onClicked: sourceDelegate.sourceBackend.removeSource(model.display)
                            }
                        }
                    }
                }
            }
        }
    }
}
