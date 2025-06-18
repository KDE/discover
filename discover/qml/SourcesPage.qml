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
            text: resourcesBackend.displayName

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

            QQC2.Label {
                text: i18n("Default Source")
                font.weight: Font.Bold
            }

            QQC2.Button {
                id: addSourceAction
                text: i18n("Add Source…")
                icon.name: "list-add-symbolic"
                visible: backendItem.backend && backendItem.backend.supportsAdding

                onClicked: {
                    const addSourceDialog = dialogComponent.createObject(window, {
                        displayName: backendItem.backend.resourcesBackend.displayName,
                    })
                    addSourceDialog.open()
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
            }


            QQC2.Button {
                id: makeDefaultAction
                visible: resourcesBackend && resourcesBackend.hasApplications && !backendItem.isDefault

                text: i18n("Make Default")
                icon.name: "favorite-symbolic"
                onClicked: Discover.ResourcesModel.currentApplicationBackend = backendItem.backend.resourcesBackend

                Component {
                    id: kirigamiAction
                    ConvertDiscoverAction {}
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

                implicitWidth: Kirigami.Units.gridUnit * 30

                Kirigami.SelectableLabel {
                    id: descriptionLabel
                    width: parent.width
                    textFormat: TextEdit.RichText
                    wrapMode: TextEdit.Wrap
                }

                footer: QQC2.DialogButtonBox {
                    QQC2.Button {
                        QQC2.DialogButtonBox.buttonRole: QQC2.DialogButtonBox.AcceptRole
                        text: i18n("Proceed")
                        icon.name: "dialog-ok"
                    }

                    QQC2.Button {
                        QQC2.DialogButtonBox.buttonRole: QQC2.DialogButtonBox.RejectRole
                        text: i18n("Cancel")
                        icon.name: "dialog-cancel"
                    }

                    onAccepted: {
                        sheet.sourcesBackend.proceed()
                        sheet.acted = true
                        sheet.close()
                    }

                    onRejected: {
                        sheet.sourcesBackend.cancel()
                        sheet.acted = true
                        sheet.close()
                    }
                }

                onOpened: {
                    descriptionLabel.forceActiveFocus(Qt.PopupFocusReason);
                }

                onClosed: {
                    if (!acted) {
                        sourcesBackend.cancel()
                    }
                    destroy();
                }
            }
        }

        delegate: QQC2.CheckDelegate {
            id: delegate

            required property int index
            required property var model
            readonly property var idx: sourcesView.model.index(index, 0)

            visible: model.display.indexOf(page.search) >= 0
            enabled: model.display.length > 0 && model.enabled && sourcesView.model.flags(idx) & Qt.ItemIsUserCheckable
            highlighted: ListView.isCurrentItem
            checkState: delegate.model.checkState

            width: ListView.view.width
            height: visible ? implicitHeight : 0

            onToggled: {
                sourcesView.model.setData(idx, checkState, Qt.CheckStateRole)
                checkState = Qt.binding(() => delegate.model.checkState)
            }

            Keys.onReturnPressed: toggle()

            contentItem: RowLayout {
                spacing: Kirigami.Units.smallSpacing

                QQC2.Label {
                    text: delegate.model.display + (delegate.model.toolTip ? " - <i>" + delegate.model.toolTip + "</i>" : "")
                    elide: Text.ElideRight
                    textFormat: Text.StyledText
                    Layout.fillWidth: true
                }

                QQC2.Button {
                    icon.name: "go-up-symbolic"
                    enabled: delegate.model.sourcesBackend.firstSourceId !== delegate.model.sourceId
                    visible: delegate.model.sourcesBackend.canMoveSources
                    onClicked: {
                        const ret = delegate.model.sourcesBackend.moveSource(delegate.model.sourceId, -1)
                        if (!ret) {
                            window.showPassiveNotification(i18n("Failed to increase '%1' preference", delegate.model.display))
                        }
                    }

                    QQC2.ToolTip.text: i18n("Increase priority")
                    QQC2.ToolTip.visible: Kirigami.Settings.tabletMode ? pressed : hovered
                    QQC2.ToolTip.delay: Kirigami.Settings.tabletMode ? Qt.styleHints.mousePressAndHoldInterval : Kirigami.Units.toolTipDelay
                }

                QQC2.Button {
                    icon.name: "go-down-symbolic"
                    enabled: delegate.model.sourcesBackend.lastSourceId !== delegate.model.sourceId
                    visible: delegate.model.sourcesBackend.canMoveSources
                    onClicked: {
                        const ret = delegate.model.sourcesBackend.moveSource(delegate.model.sourceId, +1)
                        if (!ret) {
                            window.showPassiveNotification(i18n("Failed to decrease '%1' preference", delegate.model.display))
                        }
                    }

                    QQC2.ToolTip.text: i18n("Decrease priority")
                    QQC2.ToolTip.visible: Kirigami.Settings.tabletMode ? pressed : hovered
                    QQC2.ToolTip.delay: Kirigami.Settings.tabletMode ? Qt.styleHints.mousePressAndHoldInterval : Kirigami.Units.toolTipDelay
                }

                QQC2.Button {
                    icon.name: "edit-delete-symbolic"
                    visible: delegate.model.sourcesBackend.supportsAdding
                    onClicked: {
                        const backend = delegate.model.sourcesBackend
                        if (!backend.removeSource(delegate.model.sourceId)) {
                            console.warn("Failed to remove the source", delegate.model.display)
                        }
                    }

                    QQC2.ToolTip.text: i18n("Remove repository")
                    QQC2.ToolTip.visible: Kirigami.Settings.tabletMode ? pressed : hovered
                    QQC2.ToolTip.delay: Kirigami.Settings.tabletMode ? Qt.styleHints.mousePressAndHoldInterval : Kirigami.Units.toolTipDelay
                }

                QQC2.Button {
                    icon.name: delegate.mirrored ? "go-next-symbolic-rtl" : "go-next-symbolic"
                    visible: delegate.model.sourcesBackend.canFilterSources
                    onClicked: {
                        Navigation.openApplicationListSource(delegate.model.sourceId)
                    }

                    QQC2.ToolTip.text: i18n("Show contents")
                    QQC2.ToolTip.visible: Kirigami.Settings.tabletMode ? pressed : hovered
                    QQC2.ToolTip.delay: Kirigami.Settings.tabletMode ? Qt.styleHints.mousePressAndHoldInterval : Kirigami.Units.toolTipDelay
                }
            }
        }

        footer: ColumnLayout {
            spacing: 0
            width: ListView.view.width

            Kirigami.ListSectionHeader {
                Layout.fillWidth: true

                visible: back.count > 0

                contentItem: Kirigami.Heading {
                    text: i18n("Missing Backends")
                    level: 3
                }
            }

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
                    background: null
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
