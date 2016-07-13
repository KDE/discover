import QtQuick 2.1
import QtQuick.Controls 1.1
import QtQuick.Layouts 1.1
import org.kde.discover 1.0
import org.kde.discover.app 1.0
import org.kde.kquickcontrolsaddons 2.0
import org.kde.kirigami 1.0 as Kirigami
import "navigation.js" as Navigation

Kirigami.ScrollablePage {
    id: page
    clip: true
    title: i18n("Sources")
    readonly property string icon: "view-filter"

    ListView {
        model: SourcesModel

        Menu { id: sourcesMenu }

        header: PageHeader {
            width: parent.width
            background: "https://c2.staticflickr.com/8/7460/10058518443_d0a3eb47e8_b.jpg"

            RowLayout {
                Layout.fillWidth: true
                ToolButton {
//                         iconName: "list-add"
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
                                readonly property QtObject action: modelData
                                text: action.text
                                onTriggered: action.trigger()
                                enabled: action.enabled
                            }
                        }
                    }
                }
                ToolButton {
                    action: Kirigami.Action {
                        id: configureMenu
                        text: i18n("Configure...")
//                             iconName: "settings-configure"

                        ActionBridge {
                            id: bindings
                            action: app.action("options_configure_keybinding");
                        }

                        Instantiator {
                            id: advanced
                            model: MessageActionsModel {}
                            delegate: ActionBridge { action: model.action }

                            property var objects: []
                            onObjectAdded: objects.push(object)
                            onObjectRemoved: objects = objects.splice(configureMenu.children.indexOf(object))
                        }

                        children: [sources, bindings].concat(advanced.objects)
                    }
                }

                ToolButton {
                    action: Kirigami.Action {
                        text: i18n("Help...")
//                             iconName: "system-help"

                        ActionBridge { action: app.action("help_about_app"); }
                        ActionBridge { action: app.action("help_report_bug"); }
                    }
                }
            }
        }

        delegate: ColumnLayout {
            id: sourceDelegate
            width: parent.width

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

                    RowLayout {
                        Layout.alignment: Qt.AlignVCenter
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
