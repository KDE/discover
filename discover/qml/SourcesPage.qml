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

            ToolButton {
                action: refreshAction
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

    mainItem: ListView {
        id: sourcesView
        model: QSortFilterProxyModel {
            filterRegExp: new RegExp(page.search, 'i')
            sortRole: SourcesModelClass.SourceNameRole
            sourceModel: SourcesModel
        }
        currentIndex: -1

        Component {
            id: sourceBackendDelegate
            RowLayout {
                id: backendItem
                readonly property QtObject backend: sourcesBackend
                readonly property bool isDefault: ResourcesModel.currentApplicationBackend == resourcesBackend

                anchors {
                    right: parent.right
                    left: parent.left
                }
                Kirigami.Heading {
                    Layout.fillWidth: true
                    leftPadding: Kirigami.Units.largeSpacing
                    text: backendItem.isDefault ? i18n("%1 (Default)", resourcesBackend.displayName) : resourcesBackend.displayName
                }
                Button {
                    Layout.rightMargin: Kirigami.Units.smallSpacing
                    iconName: "preferences-other"

                    Connections {
                        target: backend
                        onPassiveMessage: window.showPassiveNotification(message)
                    }

                    visible: resourcesBackend && resourcesBackend.hasApplications
                    Component {
                        id: dialogComponent
                        AddSourceDialog {
                            source: backendItem.backend
                            onVisibleChanged: if (!visible) {
                                destroy()
                            }
                        }
                    }

                    menu: Menu {
                        id: settingsMenu
                        MenuItem {
                            enabled: !backendItem.isDefault
                            text: i18n("Make default")
                            onTriggered: ResourcesModel.currentApplicationBackend = backendItem.backend.resourcesBackend
                        }

                        MenuItem {
                            text: i18n("Add Source")
                            visible: backendItem.backend

                            onTriggered: {
                                var addSourceDialog = dialogComponent.createObject(null, {displayName: backendItem.backend.resourcesBackend.displayName })
                                addSourceDialog.open()
                            }
                        }

                        MenuSeparator {
                            visible: backendActionsInst.count>0
                        }

                        Instantiator {
                            id: backendActionsInst
                            model: ActionsModel {
                                actions: backendItem.backend ? backendItem.backend.actions : undefined
                            }
                            delegate: MenuItem {
                                action: ActionBridge { action: modelData }
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

        delegate: ConditionalLoader {
            anchors {
                right: parent.right
                left: parent.left
            }
            readonly property variant resourcesBackend: model.resourcesBackend
            readonly property variant sourcesBackend: model.sourcesBackend
            readonly property variant display: model.display
            readonly property variant checked: model.checked
            readonly property variant statusTip: model.statusTip
            readonly property variant toolTip: model.toolTip
            readonly property variant modelIndex: sourcesView.model.index(index, 0)

            condition: resourcesBackend != null
            componentTrue: sourceBackendDelegate
            componentFalse: sourceDelegate
        }

        Component {
            id: sourceDelegate
            Kirigami.SwipeListItem {
                Layout.fillWidth: true
                enabled: display.length>0
                highlighted: ListView.isCurrentItem
                onClicked: Navigation.openApplicationListSource(display)

                Keys.onReturnPressed: clicked()
                actions: [
                    Kirigami.Action {
                        enabled: display.length>0
                        iconName: "view-filter"
                        tooltip: i18n("Browse the origin's resources")
                        onTriggered: Navigation.openApplicationListSource(display)
                    },
                    Kirigami.Action {
                        iconName: "edit-delete"
                        tooltip: i18n("Delete the origin")
                        onTriggered: {
                            var backend = sb
                            if (!backend.removeSource(display)) {
                                window.showPassiveNotification(i18n("Failed to remove the source '%1'", display))
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
                            sourcesView.model.setData(modelIndex, checkedState, Qt.CheckStateRole)
                        }
                    }
                    QQC2.Label {
                        text: display + " - <i>" + toolTip + "</i>"
                        elide: Text.ElideRight
                        Layout.fillWidth: true
                    }
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
                        application: application
                    }
                }
            }
        }
    }
}
