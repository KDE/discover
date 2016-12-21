import QtQuick 2.1
import QtQuick.Controls 1.1
import QtQuick.Layouts 1.1
import org.kde.discover 1.0
import org.kde.discover.app 1.0
import org.kde.kquickcontrolsaddons 2.0
import org.kde.kirigami 2.0 as Kirigami
import "navigation.js" as Navigation

DiscoverPage {
    id: page
    clip: true
    title: i18n("Settings")

    Keys.onUpPressed: sources.decrementCurrentIndex()
    Keys.onDownPressed: sources.incrementCurrentIndex()
    Keys.forwardTo: [ sources.currentItem ]

    Instantiator {
        model: SourcesModel
        delegate: QtObject {
            readonly property var sourcesModel: sourceBackend.sources

            readonly property var a: Connections {
                target: sourceBackend
                onPassiveMessage: window.showPassiveNotification(message)
            }
            readonly property var b: AddSourceDialog {
                id: addSourceDialog
                source: sourceBackend
            }

            readonly property var c: MenuItem {
                id: menuItem
                text: sourceBackend.name
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
        }
        onObjectAdded: {
            everySourceModel.addSourceModel(object.sourcesModel)
        }
        onObjectRemoved: {
            everySourceModel.removeSourceModel(object.sourcesModel)
        }
    }

    ListView {
        id: sources
        model: KConcatenateRowsProxyModel {
            id: everySourceModel
        }
        currentIndex: -1

        Menu { id: sourcesMenu }

        section {
            property: "statusTip"
            delegate: Kirigami.Heading {
                text: section
            }
        }

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


        delegate: Kirigami.SwipeListItem {
            Layout.fillWidth: true
            enabled: display.length>0
            highlighted: ListView.isCurrentItem
            onClicked: Navigation.openApplicationListSource(model.display)
            readonly property string backendName: model.statusTip

            Keys.onReturnPressed: clicked()
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
                CheckBox {
                    id: enabledBox
                    checked: model.checked != Qt.Unchecked
                    enabled: model.checked !== undefined
                    onClicked: {
                        model.checked = checkedState
                    }
                }
                Label {
                    text: model.display + " - <i>" + model.toolTip + "</i>"
                    elide: Text.ElideRight
                    Layout.fillWidth: true
                }
            }
        }
    }
}
