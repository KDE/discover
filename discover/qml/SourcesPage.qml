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

                    visible: sourcesMenu.items.count > 0
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
                    text: i18n("More...")
                    menu: actionsMenu
                    enabled: actionsMenu.items.length>0
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


            Kirigami.Heading {
                Layout.topMargin: Kirigami.Units.largeSpacing
                Layout.bottomMargin: Kirigami.Units.smallSpacing
                text: sourceBackend.name
            }
            spacing: 0
            Repeater {
                model: sourceBackend.sources

                delegate: Kirigami.SwipeListItem {
                    enabled: display.length>0
                    onClicked: Navigation.openApplicationListSource(model.display)

                    actions: [
                        Kirigami.Action {
                            enabled: display.length>0
                            iconName: "view-filter"
                            tooltip: i18n("Browse the origin's resources")
                            onTriggered: Navigation.openApplicationListSource(model.display)
                        },
                        Kirigami.Action {
                            iconName: "edit-delete"
                            tooltip: i18n("Delete the origin")
                            onTriggered: sourceDelegate.sourceBackend.removeSource(model.display)
                        }
                    ]

                    RowLayout {
                        anchors {
                            left: parent.left
                            right: parent.right
                        }
                        CheckBox {
                            id: enabledBox
                            enabled: false //TODO: implement the application of this change
                            checked: model.checked != Qt.Unchecked
                        }
                        Label {
                            text: model.display
                        }
                        Label {
                            Layout.fillWidth: true
                            text: model.toolTip
                            elide: Text.ElideRight
                        }
                    }
                }
            }
        }
    }
}
