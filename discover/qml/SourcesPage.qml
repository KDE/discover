import QtQuick 2.15
import QtQuick.Controls 2.1
import QtQuick.Layouts 1.1
import org.kde.discover 2.0
import org.kde.discover.app 1.0
import org.kde.kirigami 2.14 as Kirigami
import "navigation.js" as Navigation
import org.kde.kquickcontrolsaddons 2.0 as KQCA

DiscoverPage {
    id: page
    clip: true
    title: i18n("Settings")
    property string search: ""
    readonly property string name: title

    Kirigami.Action {
        id: configureUpdatesAction
        text: i18n("Configure Updates…")
        displayHint: Kirigami.DisplayHint.AlwaysHide
        onTriggered: {
            KQCA.KCMShell.openSystemSettings("kcm_updates");
        }
    }

    contextualActions: feedbackLoader.item ? feedbackLoader.item.actions : [configureUpdatesAction]

    header: ColumnLayout {
        Repeater {
            id: rep
            model: SourcesModel.sources
            delegate: Kirigami.InlineMessage {
                Layout.fillWidth: true
                Layout.margins: Kirigami.Units.smallSpacing
                text: modelData.inlineAction ? modelData.inlineAction.toolTip : ""
                visible: modelData.inlineAction && modelData.inlineAction.visible
                actions: Kirigami.Action {
                    icon.name: modelData.inlineAction ? modelData.inlineAction.iconName : ""
                    text: modelData.inlineAction ? modelData.inlineAction.text : ""
                    onTriggered: modelData.inlineAction.trigger()
                }

            }
        }
    }

    ListView {
        id: sourcesView
        model: SourcesModel
        Component.onCompleted: Qt.callLater(SourcesModel.showingNow)
        currentIndex: -1

        section.property: "sourceName"
        section.delegate: RowLayout {
            id: backendItem
            spacing: 0
            width: sourcesView.width
            
            readonly property QtObject backend: SourcesModel.sourcesBackendByName(section)
            readonly property QtObject resourcesBackend: backend.resourcesBackend
            readonly property bool isDefault: ResourcesModel.currentApplicationBackend === resourcesBackend

            readonly property var p0: Connections {
                target: backendItem.backend
                function onPassiveMessage(message) {
                    window.showPassiveNotification(message)
                }
                function onProceedRequest(title, description) {
                    var dialog = sourceProceedDialog.createObject(window, {sourcesBackend: backendItem.backend, title: title, description: description})
                    dialog.open()
                }
            }
            
            Kirigami.ListSectionHeader {
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignVCenter
                label: backendItem.isDefault ? i18n("%1 (Default source)", resourcesBackend.displayName) : resourcesBackend.displayName
            }
            
            Kirigami.ActionToolBar {
                id: actionBar
                Layout.leftMargin: Kirigami.Units.smallSpacing
                Layout.rightMargin: Kirigami.Units.largeSpacing
                Layout.topMargin: Kirigami.Units.smallSpacing
                alignment: Qt.AlignRight
            
                Kirigami.Action {
                    id: addSourceAction
                    text: i18n("Add Source…")
                    icon.name: "list-add"
                    visible: backendItem.backend && backendItem.backend.supportsAdding
            
                    readonly property Component p0: Component {
                        id: dialogComponent
                        AddSourceDialog {
                            source: backendItem.backend
            
                            onSheetOpenChanged: if(!sheetOpen) {
                                destroy(1000)
                            }
                        }
                    }
            
                    onTriggered: {
                        var addSourceDialog = dialogComponent.createObject(window, {displayName: backendItem.backend.resourcesBackend.displayName })
                        addSourceDialog.open()
                    }
                }
                Kirigami.Action {
                    id: makeDefaultAction
                    visible: resourcesBackend && resourcesBackend.hasApplications && !backendItem.isDefault
            
                    text: i18n("Make default")
                    icon.name: "favorite"
                    onTriggered: ResourcesModel.currentApplicationBackend = backendItem.backend.resourcesBackend
                }
            
                Component {
                    id: kirigamiAction
                    ConvertDiscoverAction {}
                }
            
                function mergeActions(moreActions) {
                    var actions = [makeDefaultAction, addSourceAction]
                    for (var i in moreActions) {
                        actions.push(kirigamiAction.createObject(actionBar, {action: moreActions[i]}))
                    }
                    return actions;
                }
                actions: mergeActions(backendItem.backend.actions)
            }
        }

        Component {
            id: sourceProceedDialog
            Kirigami.OverlaySheet {
                id: sheet
                parent: applicationWindow().overlay

                showCloseButton: false
                property QtObject  sourcesBackend
                property alias description: desc.text
                property bool acted: false

                ColumnLayout {
                    Label {
                        id: desc
                        Layout.fillWidth: true
                        textFormat: Text.StyledText
                        wrapMode: Text.WordWrap
                    }
                    RowLayout {
                        Layout.alignment: Qt.AlignRight
                        Button {
                            text: i18n("Proceed")
                            icon.name: "dialog-ok"
                            onClicked: {
                                sourcesBackend.proceed()
                                sheet.acted = true
                                sheet.close()
                            }
                        }
                        Button {
                            Layout.alignment: Qt.AlignRight
                            text: i18n("Cancel")
                            icon.name: "dialog-cancel"
                            onClicked: {
                                sourcesBackend.cancel()
                                sheet.acted = true
                                sheet.close()
                            }
                        }
                    }
                }
                onSheetOpenChanged: if(!sheetOpen) {
                    sheet.destroy(1000)
                    if (!sheet.acted)
                        sourcesBackend.cancel()
                }
            }
        }

        delegate: Kirigami.SwipeListItem {
            id: delegate
            enabled: model.display.length>0 && model.enabled
            highlighted: ListView.isCurrentItem
            supportsMouseEvents: false
            visible: model.display.indexOf(page.search)>=0
            height: visible ? implicitHeight : 0

            Keys.onReturnPressed: enabledBox.clicked()
            Keys.onSpacePressed: enabledBox.clicked()
            actions: [
                Kirigami.Action {
                    iconName: "go-up"
                    tooltip: i18n("Increase priority")
                    enabled: sourcesBackend.firstSourceId !== sourceId
                    visible: sourcesBackend.canMoveSources
                    onTriggered: {
                        var ret = sourcesBackend.moveSource(sourceId, -1)
                        if (!ret)
                            window.showPassiveNotification(i18n("Failed to increase '%1' preference", model.display))
                    }
                },
                Kirigami.Action {
                    iconName: "go-down"
                    tooltip: i18n("Decrease priority")
                    enabled: sourcesBackend.lastSourceId !== sourceId
                    visible: sourcesBackend.canMoveSources
                    onTriggered: {
                        var ret = sourcesBackend.moveSource(sourceId, +1)
                        if (!ret)
                            window.showPassiveNotification(i18n("Failed to decrease '%1' preference", model.display))
                    }
                },
                Kirigami.Action {
                    iconName: "edit-delete"
                    tooltip: i18n("Remove repository")
                    visible: sourcesBackend.supportsAdding
                    onTriggered: {
                        var backend = sourcesBackend
                        if (!backend.removeSource(sourceId)) {
                            console.warn("Failed to remove the source", model.display)
                        }
                    }
                },
                Kirigami.Action {
                    iconName: delegate.LayoutMirroring.enabled ? "go-next-symbolic-rtl" : "go-next-symbolic"
                    tooltip: i18n("Show contents")
                    visible: sourcesBackend.canFilterSources
                    onTriggered: {
                        Navigation.openApplicationListSource(sourceId)
                    }
                }
            ]

            RowLayout {
                CheckBox {
                    id: enabledBox

                    readonly property variant idx: sourcesView.model.index(index, 0)
                    readonly property variant modelChecked: model.checkState
                    checked: modelChecked !== Qt.Unchecked
                    enabled: sourcesView.model.flags(idx) & Qt.ItemIsUserCheckable
                    onClicked: {
                        sourcesView.model.setData(idx, checkState, Qt.CheckStateRole)
                        checked = Qt.binding(function() { return modelChecked !== Qt.Unchecked; })
                    }
                }
                Label {
                    text: model.display + (model.toolTip ? " - <i>" + model.toolTip + "</i>" : "")
                    elide: Text.ElideRight
                    textFormat: Text.StyledText
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
            Kirigami.Heading {
                Layout.fillWidth: true
                text: i18n("Missing Backends")
                visible: back.count>0
            }
            spacing: 0
            Repeater {
                id: back
                model: ResourcesProxyModel {
                    extending: "org.kde.discover.desktop"
                    filterMinimumState: false
                }
                delegate: Kirigami.BasicListItem {
                    supportsMouseEvents: false
                    label: name
                    icon: model.icon
                    InstallApplicationButton {
                        application: model.application
                    }
                }
            }
        }
    }
}
