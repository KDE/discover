import QtQuick 2.4
import QtQuick.Controls 1.1
import QtQuick.Controls 2.1 as QQC2
import QtQuick.Layouts 1.1
import org.kde.discover 2.0
import org.kde.discover.app 1.0
import org.kde.kquickcontrolsaddons 2.0
import org.kde.kirigami 2.1 as Kirigami
import "navigation.js" as Navigation

DiscoverPage {
    id: page
    clip: true
    title: i18n("Settings")
    property string search: ""

    readonly property var fu: Instantiator {
        model: SourcesModel
        delegate: QtObject {
            readonly property var sourcesModel: sourceBackend.sources

            readonly property var a: Connections {
                target: sourceBackend
                onPassiveMessage: window.showPassiveNotification(message)
            }
        }
        onObjectAdded: {
            everySourceModel.addSourceModel(object.sourcesModel)
        }
        onObjectRemoved: {
            everySourceModel.removeSourceModel(object.sourcesModel)
        }
    }

    mainItem: ListView {
        id: sourcesView
        model: QSortFilterProxyModel{
            filterRegExp: new RegExp(page.search, 'i')
            sourceModel: KConcatenateRowsProxyModel {
                id: everySourceModel
            }
        }
        currentIndex: -1

        section {
            property: "statusTip"
            delegate: RowLayout {
                anchors {
                    right: parent.right
                    left: parent.left
                }
                Kirigami.Heading {
                    Layout.fillWidth: true
                    leftPadding: Kirigami.Units.largeSpacing
                    text: settingsButton.isDefault ? i18n("%1 (Default)", section) : section
                }
                Button {
                    id: settingsButton
                    Layout.rightMargin: Kirigami.Units.smallSpacing
                    iconName: "preferences-other"
                    readonly property QtObject backend: SourcesModel.backendForSection(section)
                    readonly property bool isDefault: ResourcesModel.currentApplicationBackend == settingsButton.backend.resourcesBackend
                    visible: backend
                    AddSourceDialog {
                        id: addSourceDialog
                        source: settingsButton.backend
                    }

                    menu: Menu {
                        id: settingsMenu
                        MenuItem {
                            enabled: !settingsButton.isDefault
                            text: i18n("Make default")
                            onTriggered: ResourcesModel.currentApplicationBackend = settingsButton.backend.resourcesBackend
                        }

                        MenuItem {
                            text: i18n("Add Source")

                            onTriggered: addSourceDialog.open()
                        }

                        MenuSeparator {
                            visible: messageActionsInst.count>0
                        }

                        Instantiator {
                            id: messageActionsInst
                            model: ActionsModel {
                                actions: settingsButton.backend ? settingsButton.backend.resourcesBackend.messageActions : null
                            }
                            delegate: MenuItem {
                                action: ActionBridge { action: model.action }
                            }
                            onObjectAdded: {
                                settingsMenu.insertItem(index, object)
                            }
                            onObjectRemoved: {
                                object.destroy()
                            }
                        }

                        MenuSeparator {
                            visible: backendActionsInst.count>0
                        }

                        Instantiator {
                            id: backendActionsInst
                            model: ActionsModel {
                                actions: settingsButton.backend ? settingsButton.backend.actions : null
                            }
                            delegate: MenuItem {
                                action: ActionBridge { action: model.action }
                            }
                            onObjectAdded: {
                                settingsMenu.insertItem(index, object)
                            }
                            onObjectRemoved: {
                                object.destroy()
                            }
                        }
                    }
                }
            }
        }

        headerPositioning: ListView.OverlayHeader
        header: QQC2.ToolBar {
            anchors {
                right: parent.right
                left: parent.left
            }

            contentItem: RowLayout {
                anchors {
                    topMargin: Kirigami.Units.smallSpacing
                    bottomMargin: Kirigami.Units.smallSpacing
                }

                Item {
                    Layout.fillWidth: true
                }

                Repeater {
                    model: SourcesModel.actions

                    delegate: RowLayout {
                        Kirigami.Icon {
                            source: modelData.icon
                        }
                        visible: theAction.action && theAction.action.visible
                        ToolButton {
                            height: parent.height
                            action: Action {
                                id: theAction
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


        delegate: Kirigami.SwipeListItem {
            Layout.fillWidth: true
            enabled: display.length>0
            highlighted: ListView.isCurrentItem
            onClicked: Navigation.openApplicationListSource(model.display)
            readonly property string backendName: model.statusTip
            readonly property variant modelIndex: sourcesView.model.index(model.index, 0)

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
                    onTriggered: {
                        var backend = sourcesView.model.data(modelIndex, AbstractSourcesBackend.SourcesBackend)
                        if (!backend.removeSource(model.display)) {
                            window.showPassiveNotification(i18n("Failed to remove the source '%1'", model.display))
                        }
                    }
                }
            ]

            RowLayout {
                CheckBox {
                    id: enabledBox

                    readonly property variant modelChecked: sourcesView.model.data(modelIndex, Qt.CheckStateRole)
                    checked: modelChecked != Qt.Unchecked
                    enabled: modelChecked !== undefined
                    onClicked: {
                        model.checked = checkedState
                    }
                }
                QQC2.Label {
                    text: model.display + " - <i>" + model.toolTip + "</i>"
                    elide: Text.ElideRight
                    Layout.fillWidth: true
                }
            }
        }

        footer: ColumnLayout {
            id: foot
            anchors {
                right: parent.right
                left: parent.left
                margins: Kirigami.Units.smallSpacing
            }
            Repeater {
                id: back
                model: ResourcesProxyModel {
                    extending: "org.kde.discover.desktop"
                }
                delegate: RowLayout {
                    visible: !model.application.isInstalled
                    QQC2.Label {
                        Layout.fillWidth: true
                        text: name
                    }
                    InstallApplicationButton {
                        application: model.application
                    }
                }
            }
        }
    }
}
