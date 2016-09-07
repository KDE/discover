import QtQuick 2.1
import QtQuick.Controls 1.1
import QtQuick.Layouts 1.1
import org.kde.discover 1.0
import org.kde.discover.app 1.0
import org.kde.kquickcontrolsaddons 2.0
import org.kde.kirigami 1.0 as Kirigami
import "navigation.js" as Navigation

DiscoverPage {
    id: page
    clip: true
    title: i18n("Settings")

    ListView {
        model: SourcesModel

        Menu { id: sourcesMenu }

        header: PageHeader {
            anchors {
                left: parent.left
                right: parent.right
            }

            RowLayout {
                Layout.fillWidth: true
                Layout.leftMargin: Kirigami.Units.gridUnit
                Layout.rightMargin: Kirigami.Units.gridUnit
                Layout.topMargin: Kirigami.Units.smallSpacing
                Layout.bottomMargin: Kirigami.Units.smallSpacing
                ToolButton {
//                         iconName: "list-add"
                    text: i18n("Add Source")

                    enabled: sourcesMenu.items.count > 0
                    tooltip: text
                    menu: sourcesMenu
                }
                Repeater {
                    model: SourcesModel.actions

                    delegate: RowLayout {
                        QIconItem {
                            icon: modelData.icon
                        }
                        ToolButton {
                            height: parent.height
                            action: Action {
                                readonly property QtObject action: modelData
                                text: action.text
                                onTriggered: action.trigger()
                                enabled: action.enabled
                            }
                        }
                    }
                }

                ToolButton {
                    text: i18n("Help...")
                    menu: Menu {
                        MenuItem { action: ActionBridge { action: app.action("help_about_app") } }
                        MenuItem { action: ActionBridge { action: app.action("help_report_bug") } }
                    }
                }
            }
        }

        delegate: ColumnLayout {
            id: sourceDelegate
            anchors {
                left: parent.left
                right: parent.right
                leftMargin: Kirigami.Units.largeSpacing
                rightMargin: Kirigami.Units.largeSpacing
            }

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

                delegate: Kirigami.AbstractListItem {
                    height: browseOrigin.implicitHeight*1.4
                    enabled: browseOrigin.enabled
                    onClicked: Navigation.openApplicationListSource(model.display)
                    Layout.fillWidth: true

                    RowLayout {
                        Layout.alignment: Qt.AlignVCenter
                        Layout.fillWidth: true

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
                            enabled: display.length>0
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
