pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.discover as Discover
import org.kde.kcmutils as KCMUtils
import org.kde.kirigami as Kirigami
import org.kde.kirigami.delegates as KD

DiscoverPage {
    id: page

    property string search
    readonly property string name: title

    clip: true
    title: i18n("Settings")

    Kirigami.Action {
        id: configureUpdatesAction
        text: i18n("Configure Updates…")
        displayHint: Kirigami.DisplayHint.AlwaysHide
        onTriggered: {
            KCMUtils.KCMLauncher.openSystemSettings("kcm_updates");
        }
    }

    actions: feedbackLoader.item?.actions ?? [configureUpdatesAction]

    header: ColumnLayout {
        spacing: Kirigami.Units.smallSpacing

        Repeater {
            model: Discover.SourcesModel.sources

            delegate: Kirigami.InlineMessage {
                id: delegate

                required property Discover.AbstractSourcesBackend modelData

                Layout.fillWidth: true
                Layout.margins: Kirigami.Units.smallSpacing
                text: modelData.inlineAction?.toolTip ?? ""
                visible: modelData.inlineAction?.visible ?? false
                actions: Kirigami.Action {
                    icon.name: delegate.modelData.inlineAction?.iconName ?? ""
                    text: delegate.modelData.inlineAction?.text ?? ""
                    onTriggered: delegate.modelData.inlineAction?.trigger()
                }
            }
        }
    }

    ListView {
        id: sourcesView
        model: Discover.SourcesModel
        Component.onCompleted: Qt.callLater(Discover.SourcesModel.showingNow)
        currentIndex: -1
        pixelAligned: true
        section.property: "sourceName"
        section.delegate: Kirigami.ListSectionHeader {
            id: backendItem

            required property string section

            height: Math.ceil(Math.max(Kirigami.Units.gridUnit * 2.5, contentItem.implicitHeight))

            readonly property Discover.AbstractSourcesBackend backend: Discover.SourcesModel.sourcesBackendByName(section)
            readonly property Discover.AbstractResourcesBackend resourcesBackend: backend.resourcesBackend
            readonly property bool isDefault: Discover.ResourcesModel.currentApplicationBackend === resourcesBackend

            width: sourcesView.width

            Connections {
                target: backendItem.backend
                function onPassiveMessage(message) {
                    window.showPassiveNotification(message)
                }
                function onProceedRequest(title, description) {
                    const dialog = sourceProceedDialog.createObject(window, {
                        sourcesBackend: backendItem.backend,
                        title,
                        description,
                    })
                    dialog.open()
                }
            }

            contentItem: RowLayout {
                spacing: Kirigami.Units.smallSpacing

                Kirigami.Heading {
                    text: resourcesBackend.displayName
                    level: 3
                    font.weight: backendItem.isDefault ? Font.Bold : Font.Normal
                }

                Kirigami.ActionToolBar {
                    id: actionBar

                    alignment: Qt.AlignRight

                    Kirigami.Action {
                        id: isDefaultbackendLabelAction

                        visible: backendItem.isDefault
                        displayHint: Kirigami.DisplayHint.KeepVisible
                        displayComponent: Kirigami.Heading {
                            text: i18n("Default source")
                            level: 3
                            font.weight: Font.Bold
                        }
                    }

                    Kirigami.Action {
                        id: addSourceAction
                        text: i18n("Add Source…")
                        icon.name: "list-add"
                        visible: backendItem.backend && backendItem.backend.supportsAdding

                        onTriggered: {
                            const addSourceDialog = dialogComponent.createObject(window, {
                                displayName: backendItem.backend.resourcesBackend.displayName,
                            })
                            addSourceDialog.open()
                        }
                    }

                    Component {
                        id: dialogComponent
                        AddSourceDialog {
                            source: backendItem.backend

                            onClosed: {
                                destroy();
                            }
                        }
                    }

                    Kirigami.Action {
                        id: makeDefaultAction
                        visible: resourcesBackend && resourcesBackend.hasApplications && !backendItem.isDefault

                        text: i18n("Make default")
                        icon.name: "favorite"
                        onTriggered: Discover.ResourcesModel.currentApplicationBackend = backendItem.backend.resourcesBackend
                    }

                    Component {
                        id: kirigamiAction
                        ConvertDiscoverAction {}
                    }

                    function mergeActions(moreActions) {
                        const actions = [
                            isDefaultbackendLabelAction,
                            makeDefaultAction,
                            addSourceAction
                        ]
                        for (const action of moreActions) {
                            actions.push(kirigamiAction.createObject(this, { action }))
                        }
                        return actions;
                    }
                    actions: mergeActions(backendItem.backend.actions)
                }
            }
        }

        Component {
            id: sourceProceedDialog
            Kirigami.OverlaySheet {
                id: sheet

                property Discover.AbstractSourcesBackend sourcesBackend
                property alias description: descriptionLabel.text
                property bool acted: false

                parent: page.QQC2.Overlay.overlay
                showCloseButton: false

                ColumnLayout {
                    QQC2.Label {
                        id: descriptionLabel
                        Layout.fillWidth: true
                        textFormat: Text.StyledText
                        wrapMode: Text.WordWrap
                    }
                    RowLayout {
                        Layout.alignment: Qt.AlignRight
                        QQC2.Button {
                            text: i18n("Proceed")
                            icon.name: "dialog-ok"
                            onClicked: {
                                sheet.sourcesBackend.proceed()
                                sheet.acted = true
                                sheet.close()
                            }
                        }
                        QQC2.Button {
                            Layout.alignment: Qt.AlignRight
                            text: i18n("Cancel")
                            icon.name: "dialog-cancel"
                            onClicked: {
                                sheet.sourcesBackend.cancel()
                                sheet.acted = true
                                sheet.close()
                            }
                        }
                    }
                }

                onVisibleChanged: if (!visible) {
                    sheet.destroy(1000)
                    if (!sheet.acted) {
                        sourcesBackend.cancel()
                    }
                }
            }
        }

        delegate: Kirigami.SwipeListItem {
            id: delegate

            required property int index
            required property var model

            enabled: model.display.length > 0 && model.enabled
            highlighted: ListView.isCurrentItem
            supportsMouseEvents: false
            visible: model.display.indexOf(page.search) >= 0
            height: visible ? implicitHeight : 0

            Keys.onReturnPressed: enabledBox.clicked()
            Keys.onSpacePressed: enabledBox.clicked()
            actions: [
                Kirigami.Action {
                    icon.name: "go-up"
                    tooltip: i18n("Increase priority")
                    enabled: delegate.model.sourcesBackend.firstSourceId !== delegate.model.sourceId
                    visible: delegate.model.sourcesBackend.canMoveSources
                    onTriggered: {
                        const ret = delegate.model.sourcesBackend.moveSource(delegate.model.sourceId, -1)
                        if (!ret) {
                            window.showPassiveNotification(i18n("Failed to increase '%1' preference", delegate.model.display))
                        }
                    }
                },
                Kirigami.Action {
                    icon.name: "go-down"
                    tooltip: i18n("Decrease priority")
                    enabled: delegate.model.sourcesBackend.lastSourceId !== delegate.model.sourceId
                    visible: delegate.model.sourcesBackend.canMoveSources
                    onTriggered: {
                        const ret = delegate.model.sourcesBackend.moveSource(delegate.model.sourceId, +1)
                        if (!ret) {
                            window.showPassiveNotification(i18n("Failed to decrease '%1' preference", delegate.model.display))
                        }
                    }
                },
                Kirigami.Action {
                    icon.name: "edit-delete"
                    tooltip: i18n("Remove repository")
                    visible: delegate.model.sourcesBackend.supportsAdding
                    onTriggered: {
                        const backend = delegate.model.sourcesBackend
                        if (!backend.removeSource(delegate.model.sourceId)) {
                            console.warn("Failed to remove the source", delegate.model.display)
                        }
                    }
                },
                Kirigami.Action {
                    icon.name: delegate.mirrored ? "go-next-symbolic-rtl" : "go-next-symbolic"
                    tooltip: i18n("Show contents")
                    visible: delegate.model.sourcesBackend.canFilterSources
                    onTriggered: {
                        Navigation.openApplicationListSource(delegate.model.sourceId)
                    }
                }
            ]

            contentItem: RowLayout {
                spacing: Kirigami.Units.smallSpacing

                QQC2.CheckBox {
                    id: enabledBox

                    readonly property var idx: sourcesView.model.index(index, 0)
                    readonly property /*Qt::CheckState*/int modelChecked: delegate.model.checkState
                    checked: modelChecked !== Qt.Unchecked
                    enabled: sourcesView.model.flags(idx) & Qt.ItemIsUserCheckable
                    onClicked: {
                        sourcesView.model.setData(idx, checkState, Qt.CheckStateRole)
                        checked = Qt.binding(() => (modelChecked !== Qt.Unchecked))
                    }
                }
                QQC2.Label {
                    text: delegate.model.display + (delegate.model.toolTip ? " - <i>" + delegate.model.toolTip + "</i>" : "")
                    elide: Text.ElideRight
                    textFormat: Text.StyledText
                    Layout.fillWidth: true
                }
            }
        }

        footer: ColumnLayout {
            anchors {
                right: parent.right
                left: parent.left
                margins: Kirigami.Units.smallSpacing
            }
            Kirigami.ListSectionHeader {
                contentItem: Kirigami.Heading {
                    Layout.fillWidth: true
                    text: i18n("Missing Backends")
                    visible: back.count > 0
                    level: 3
                }
            }
            spacing: 0
            Repeater {
                id: back
                model: Discover.ResourcesProxyModel {
                    extending: "org.kde.discover.desktop"
                    filterMinimumState: false
                    stateFilter: Discover.AbstractResource.None
                }
                delegate: QQC2.ItemDelegate {
                    id: delegate

                    required property int index
                    required property var model

                    Layout.fillWidth: true
                    hoverEnabled: false
                    down: false

                    contentItem: RowLayout {
                        spacing: Kirigami.Units.smallSpacing
                        KD.IconTitleSubtitle {
                            title: name
                            icon.source: delegate.model.icon
                            subtitle: delegate.model.comment
                            Layout.fillWidth: true
                        }
                        InstallApplicationButton {
                            application: delegate.model.application
                        }
                    }
                }
            }
        }
    }
}
