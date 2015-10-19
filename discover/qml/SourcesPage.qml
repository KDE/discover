import QtQuick 2.1
import QtQuick.Controls 1.1
import QtQuick.Layouts 1.1
import org.kde.muon 1.0
import org.kde.kquickcontrolsaddons 2.0
import "navigation.js" as Navigation

Item {
    id: page
    clip: true
    readonly property real proposedMargin: (width-app.actualWidth)/2
    readonly property string title: i18n("Sources")
    readonly property string icon: "view-filter"

    Menu {
        id: sourcesMenu
    }

    ScrollView {
        anchors.fill: parent
        ListView {
            id: view
            width: parent.width

            model: SourcesModel

            header: PageHeader {
                x: page.proposedMargin
                width: app.actualWidth
                hoverEnabled: false
                RowLayout {
                    anchors.verticalCenter: parent.verticalCenter
                    ToolButton {
                        iconName: "list-add"
                        text: i18n("Add Source")

                        tooltip: text
                        menu: sourcesMenu
                    }
                    Repeater {
                        model: SourcesModel.actions

                        delegate: RowLayout{
                            QIconItem {
                                icon: modelData.icon
                            }
                            ToolButton {
                                height: parent.height
                                action: Action {
                                    property QtObject action: modelData
                                    text: action.text
                                    onTriggered: action.trigger()
                                    enabled: action.enabled
                                }
                            }
                        }
                    }
                }
            }

            delegate: ColumnLayout {
                id: sourceDelegate
                x: page.proposedMargin
                width: app.actualWidth
                spacing: -2

                property QtObject sourceBackend: model.sourceBackend
                AddSourceDialog {
                    id: addSourceDialog
                    source: sourceDelegate.sourceBackend
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
                        Layout.fillWidth: true
                        height: browseOrigin.implicitHeight*1.4
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
                                tooltip: i18n("Browse the origin's resources")
                                onClicked: Navigation.openApplicationListSource(model.display)
                            }
                            Button {
                                iconName: "edit-delete"
                                onClicked: sourceDelegate.sourceBackend.removeSource(model.display)
                                tooltip: i18n("Delete the origin")
                            }
                        }
                    }
                }
            }
        }
    }
}
