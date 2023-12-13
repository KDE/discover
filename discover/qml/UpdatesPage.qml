pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.discover as Discover
import org.kde.discover.app as DiscoverApp
import org.kde.kirigami as Kirigami

DiscoverPage {
    id: page

    title: i18n("Updates")

    property string footerLabel: ""
    property int footerProgress: 0
    property bool busy: false
    readonly property string name: title

    Discover.ResourcesUpdatesModel {
        id: resourcesUpdatesModel
        onPassiveMessage: {
            sheet.errorMessage = message;
            sheet.visible = true;
        }
        onIsProgressingChanged: {
            if (!isProgressing) {
                resourcesUpdatesModel.prepare()
            }
        }

        Component.onCompleted: {
            if (!isProgressing) {
                resourcesUpdatesModel.prepare()
            }
        }
    }

    Kirigami.OverlaySheet {
        id: sheet

        property string errorMessage: ""

        parent: page.QQC2.Overlay.overlay

        title: contentLoader.sourceComponent === friendlyMessageComponent ? i18n("Update Issue") :  i18n("Technical details")

        Loader {
            id: contentLoader
            active: true
            sourceComponent: friendlyMessageComponent

            Component {
                id: friendlyMessageComponent

                ColumnLayout {
                    QQC2.Label {
                        id: friendlyMessage
                        Layout.fillWidth: true
                        Layout.maximumWidth: Math.round(page.width * 0.75)
                        Layout.bottomMargin: Kirigami.Units.largeSpacing * 2
                        text: i18n("There was an issue installing this update. Please try again later.")
                        wrapMode: Text.WordWrap
                    }
                    QQC2.Button {
                        id: seeDetailsAndreportIssueButton
                        Layout.alignment: Qt.AlignRight
                        text: i18n("See Technical Details")
                        icon.name: "view-process-system"
                        onClicked: {
                            contentLoader.sourceComponent = nerdyDetailsComponent;
                        }
                    }
                }
            }

            Component {
                id: nerdyDetailsComponent

                ColumnLayout {
                    spacing: Kirigami.Units.smallSpacing

                    QQC2.TextArea {
                        Layout.fillWidth: true
                        text: sheet.errorMessage
                        textFormat: TextEdit.RichText
                        wrapMode: TextEdit.Wrap
                    }

                    QQC2.Label {
                        Layout.fillWidth: true
                        Layout.maximumWidth: Math.round(page.width*0.75)
                        Layout.topMargin: Kirigami.Units.largeSpacing
                        Layout.bottomMargin: Kirigami.Units.largeSpacing
                        text: i18nc("@info %1 is the name of the user's distro/OS", "If the error indicated above looks like a real issue and not a temporary network error, please report it to %1, not KDE.", Discover.ResourcesModel.distroName)
                        wrapMode: Text.WordWrap
                    }

                    RowLayout {
                        Layout.fillWidth: true

                        QQC2.Button {
                            text: i18n("Copy Text")
                            icon.name: "edit-copy"
                            onClicked: {
                                app.copyTextToClipboard(sheet.errorMessage);
                                window.showPassiveNotification(i18nc("@info %1 is the name of the user's distro/OS", "Error message copied. Remember to report it to %1, not KDE!", Discover.ResourcesModel.distroName));
                            }
                        }

                        Item { Layout.fillWidth: true}

                        QQC2.Button {
                            text: i18nc("@action:button %1 is the name of the user's distro/OS", "Report Issue to %1", Discover.ResourcesModel.distroName)
                            icon.name: "tools-report-bug"
                            onClicked: {
                                Qt.openUrlExternally(Discover.ResourcesModel.distroBugReportUrl())
                                sheet.visible = false
                            }
                        }
                    }
                }
            }
        }

        // Ensure that friendly message is shown if the user closes the sheet and
        // then opens it again
        onVisibleChanged: if (visible) {
            contentLoader.sourceComponent = friendlyMessageComponent;
        }
    }

    Discover.UpdateModel {
        id: updateModel
        backend: resourcesUpdatesModel
    }

    Kirigami.Action {
        id: updateAction
        text: page.unselected > 0 ? i18nc("@action:button as in, 'update the selected items' ", "Update Selected") : i18nc("@action:button as in, 'update all items'", "Update All")
        visible: updateModel.toUpdateCount
        icon.name: "update-none"

        function anyVisible(items) {
            for (const itemPos in items) {
                const item = items[itemPos];
                if (item.visible && item instanceof Kirigami.InlineMessage) {
                    return true
                }
            }
            return false;
        }

        enabled: !resourcesUpdatesModel.isProgressing && !Discover.ResourcesModel.isFetching && !anyVisible(page.header.children)
        onTriggered: resourcesUpdatesModel.updateAll()
    }

    header: ColumnLayout {
        id: errorsColumn

        spacing: Kirigami.Units.smallSpacing

        DiscoverInlineMessage {
            Layout.fillWidth: true
            Layout.margins: Kirigami.Units.smallSpacing
            inlineMessage: Discover.ResourcesModel.inlineMessage
        }

        Repeater {
            model: resourcesUpdatesModel.errorMessages
            delegate: Kirigami.InlineMessage {
                id: inline

                required property string modelData

                Layout.fillWidth: true
                Layout.margins: Kirigami.Units.smallSpacing
                text: modelData
                visible: true
                type: Kirigami.MessageType.Error
                onVisibleChanged: errorsColumn.childrenChanged()

                actions: [
                    Kirigami.Action {
                        icon.name: "dialog-cancel"
                        text: i18n("Ignore")
                        onTriggered: {
                            inline.visible = false
                        }
                    }
                ]
            }
        }
    }

    footer: ColumnLayout {
        width: parent.width
        spacing: 0

        QQC2.ScrollView {
            id: scv
            Layout.fillWidth: true
            Layout.preferredHeight: visible ? Kirigami.Units.gridUnit * 10 : 0
            visible: log.contents.length > 0
            QQC2.TextArea {
                readOnly: true
                text: log.contents
                wrapMode: TextEdit.Wrap

                cursorPosition: text.length - 1
                font.family: "monospace"

                Discover.ReadFile {
                    id: log
                    filter: ".*ALPM-SCRIPTLET\\] .*"
                    path: "/var/log/pacman.log"
                }
            }
        }

        QQC2.ToolBar {
            id: footerToolbar
            Layout.fillWidth: true
            visible: (updateModel.totalUpdatesCount > 0 && resourcesUpdatesModel.isProgressing) || updateModel.hasUpdates

            position: QQC2.ToolBar.Footer

            contentItem: RowLayout {
                QQC2.ToolButton {
                    enabled: page.unselected > 0 && updateAction.enabled && !Discover.ResourcesModel.isFetching
                    visible: updateModel.totalUpdatesCount > 1 && !resourcesUpdatesModel.isProgressing
                    icon.name: "edit-select-all"
                    text: i18n("Select All")
                    onClicked: { updateModel.checkAll(); }
                }

                QQC2.ToolButton {
                    enabled: page.unselected !== updateModel.totalUpdatesCount && updateAction.enabled && !Discover.ResourcesModel.isFetching
                    visible: updateModel.totalUpdatesCount > 1 && !resourcesUpdatesModel.isProgressing
                    icon.name: "edit-select-none"
                    text: i18n("Select None")
                    onClicked: { updateModel.uncheckAll(); }
                }

                QQC2.CheckBox {
                    id: rebootAtEnd
                    visible: resourcesUpdatesModel.needsReboot && resourcesUpdatesModel.isProgressing
                    text: i18n("Restart automatically after update has completed");
                }

                QQC2.Label {
                    Layout.fillWidth: true
                    Layout.rightMargin: Kirigami.Units.largeSpacing
                    horizontalAlignment: Text.AlignRight
                    text: i18n("Total size: %1", updateModel.updateSize)
                    elide: Text.ElideLeft
                }
            }
        }
    }

    Kirigami.Action {
        id: cancelUpdateAction
        icon.name: "dialog-cancel"
        text: i18n("Cancel")
        enabled: resourcesUpdatesModel.transaction && resourcesUpdatesModel.transaction.isCancellable
        onTriggered: resourcesUpdatesModel.transaction.cancel()
    }

    readonly property int unselected: (updateModel.totalUpdatesCount - updateModel.toUpdateCount)

    supportsRefreshing: true
    onRefreshingChanged: {
        Discover.ResourcesModel.updateAction.triggered()
        refreshing = false
    }

    readonly property Item report: ColumnLayout {
        parent: page
        anchors.fill: parent
        anchors.margins: Kirigami.Units.largeSpacing * 2
        Item {
            Layout.fillHeight: true
            width: 1
        }

        Kirigami.Action {
            id: restartAction
            icon.name: "system-reboot"
            text: i18n("Restart Now")
            visible: false
            onTriggered: app.reboot()
        }
        Kirigami.LoadingPlaceholder {
            id: statusLabel
            icon.name: {
                if (page.footerProgress === 0 && page.footerLabel !== "" && !page.busy) {
                    return "update-none"
                } else {
                    return ""
                }
            }
            text: page.footerLabel
            determinate: true
            progressBar.value: page.footerProgress
        }

        Item {
            Layout.fillHeight: true
            width: 1
        }
    }
    ListView {
        id: updatesView
        currentIndex: -1
        reuseItems: true
        clip: true

        model: DiscoverApp.QSortFilterProxyModel {
            sourceModel: updateModel
            sortRole: Discover.UpdateModel.SectionResourceProgressRole
        }

        section {
            property: "section"
            delegate: Kirigami.ListSectionHeader {
                required property string section

                width: updatesView.width
                label: section
            }
        }

        delegate: QQC2.ItemDelegate {
            id: listItem

            // type: roles of Discover.UpdateModel
            required property var model
            required property int index
            required property bool extended

            width: updatesView.width

            highlighted: false

            onEnabledChanged: if (!enabled) {
                model.extended = false;
            }

            visible: model.resourceState < Discover.AbstractBackendUpdater.Done

            Keys.onReturnPressed: event => {
                itemChecked.clicked();
            }
            Keys.onPressed: event => {
                if (event.key === Qt.Key_Alt) {
                    model.extended = true;
                }
            }
            Keys.onReleased: event => {
                if (event.key === Qt.Key_Alt) {
                    model.extended = false;
                }
            }

            onExtendedChanged: if (extended) {
                updateModel.fetchUpdateDetails(index)
            }

            contentItem: ColumnLayout {
                spacing: Kirigami.Units.smallSpacing

                RowLayout {
                    spacing: Kirigami.Units.smallSpacing
                    Layout.fillWidth: true
                    Layout.fillHeight: true

                    QQC2.CheckBox {
                        id: itemChecked
                        Layout.alignment: Qt.AlignVCenter
                        checked: listItem.model.checked === Qt.Checked
                        onClicked: listItem.model.checked = (listItem.model.checked === Qt.Checked ? Qt.Unchecked : Qt.Checked)
                        enabled: !resourcesUpdatesModel.isProgressing
                    }

                    Kirigami.Icon {
                        width: Kirigami.Units.gridUnit * 2
                        Layout.preferredHeight: width
                        source: listItem.model.decoration
                        smooth: true
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        Layout.leftMargin: Kirigami.Units.smallSpacing
                        Layout.alignment: Qt.AlignVCenter

                        spacing: 0

                        // App name
                        Kirigami.Heading {
                            Layout.fillWidth: true
                            text: listItem.model.display
                            level: 3
                            elide: Text.ElideRight
                        }

                        // Version numbers
                        QQC2.Label {
                            Layout.fillWidth: true
                            elide: truncated ? Text.ElideLeft : Text.ElideRight
                            text: listItem.model.resource.upgradeText
                            opacity: listItem.hovered ? 0.8 : 0.6
                        }
                    }

                    LabelBackground {
                        Layout.minimumWidth: Kirigami.Units.gridUnit * 6
                        text: listItem.model.resourceState === 2 ? i18n("Installing") : listItem.model.size

                        progress: listItem.model.resourceProgress / 100
                    }
                }

                QQC2.Frame {
                    Layout.fillWidth: true
                    implicitHeight: view.contentHeight
                    visible: listItem.model.extended && listItem.model.changelog.length > 0
                    QQC2.Label {
                        id: view
                        anchors {
                            right: parent.right
                            left: parent.left
                        }
                        text: listItem.model.changelog
                        textFormat: Text.StyledText
                        wrapMode: Text.WordWrap
                        onLinkActivated: link => Qt.openUrlExternally(link)

                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: Kirigami.Units.smallSpacing
                    visible: listItem.model.extended

                    QQC2.Label {
                        Layout.leftMargin: Kirigami.Units.gridUnit
                        text: i18n("Update from:")
                    }
                    // Backend icon
                    Kirigami.Icon {
                        source: listItem.model.resource.sourceIcon
                        implicitWidth: Kirigami.Units.iconSizes.smallMedium
                        implicitHeight: Kirigami.Units.iconSizes.smallMedium
                    }
                    // Backend label and origin/remote
                    QQC2.Label {
                        Layout.fillWidth: true
                        text: listItem.model.resource.origin.length === 0 ? listItem.model.resource.backend.displayName
                                : i18nc("%1 is the backend that provides this app, %2 is the specific repository or address within that backend","%1 (%2)",
                                        listItem.model.resource.backend.displayName, listItem.model.resource.origin)
                        elide: Text.ElideRight
                    }

                    QQC2.Button {
                        Layout.alignment: Qt.AlignRight
                        text: i18n("More Information…")
                        enabled: !resourcesUpdatesModel.isProgressing
                        onClicked: Navigation.openApplication(listItem.model.resource)
                    }
                }
            }

            onClicked: {
                model.extended = !model.extended
            }
        }
    }

    readonly property alias secSinceUpdate: resourcesUpdatesModel.secsToLastUpdate
    state:  ( resourcesUpdatesModel.isProgressing        ? "progressing"
            : Discover.ResourcesModel.isFetching         ? "fetching"
            : updateModel.hasUpdates                     ? "has-updates"
            : resourcesUpdatesModel.needsReboot          ? "reboot"
            : secSinceUpdate < 0                         ? "unknown"
            : secSinceUpdate === 0                       ? "now-uptodate"
            : secSinceUpdate < 1000 * 60 * 60 * 24       ? "uptodate"
            : secSinceUpdate < 1000 * 60 * 60 * 24 * 7   ? "medium"
            :                                              "low"
            )

    states: [
        State {
            name: "fetching"
            PropertyChanges { target: page; footerLabel: i18nc("@info", "Fetching updates…") }
            PropertyChanges { target: page; footerProgress: Discover.ResourcesModel.fetchingUpdatesProgress }
            PropertyChanges { target: page; busy: true }
            PropertyChanges { target: updatesView; opacity: 0 }
        },
        State {
            name: "progressing"
            PropertyChanges { target: page; supportsRefreshing: false }
            PropertyChanges { target: page; actions: [cancelUpdateAction] }
            PropertyChanges { target: statusLabel; visible: false }
        },
        State {
            name: "has-updates"
            PropertyChanges { target: page; title: i18nc("@info", "Updates") }
            // On mobile, we want "Update" to be the primary action so it's in
            // the center, but on desktop this feels a bit awkward and it would
            // be better to have "Update" be the right-most action
            PropertyChanges { target: page; actions: [ updateAction, refreshAction ] }
            PropertyChanges { target: statusLabel; visible: false }
        },
        State {
            name: "reboot"
            PropertyChanges { target: page; footerLabel: i18nc("@info", "Restart the system to complete the update process") }
            PropertyChanges { target: statusLabel; helpfulAction: restartAction }
            PropertyChanges { target: statusLabel; explanation: "" }
            PropertyChanges { target: statusLabel.progressBar; visible: false }
            StateChangeScript {
                script: if (rebootAtEnd.checked && resourcesUpdatesModel.readyToReboot) {
                    app.rebootNow()
                }
            }
        },
        State {
            name: "now-uptodate"
            PropertyChanges { target: page; footerLabel: i18nc("@info", "Up to date") }
            PropertyChanges { target: page; actions: [refreshAction] }
            PropertyChanges { target: statusLabel; explanation: "" }
            PropertyChanges { target: statusLabel.progressBar; visible: false }
        },
        State {
            name: "uptodate"
            PropertyChanges { target: page; footerLabel: i18nc("@info", "Up to date") }
            PropertyChanges { target: page; actions: [refreshAction] }
            PropertyChanges { target: statusLabel; explanation: "" }
            PropertyChanges { target: statusLabel.progressBar; visible: false }
        },
        State {
            name: "medium"
            PropertyChanges { target: page; title: i18nc("@info", "Up to date") }
            PropertyChanges { target: page; actions: [refreshAction] }
            PropertyChanges { target: statusLabel; explanation: "" }
            PropertyChanges { target: statusLabel.progressBar; visible: false }
        },
        State {
            name: "low"
            PropertyChanges { target: page; title: i18nc("@info", "Should check for updates") }
            PropertyChanges { target: page; actions: [refreshAction] }
            PropertyChanges { target: statusLabel; explanation: "" }
            PropertyChanges { target: statusLabel.progressBar; visible: false }
        },
        State {
            name: "unknown"
            PropertyChanges { target: page; title: i18nc("@info", "Time of last update unknown") }
            PropertyChanges { target: page; actions: [refreshAction] }
            PropertyChanges { target: statusLabel; explanation: "" }
            PropertyChanges { target: statusLabel.progressBar; visible: false }
        }
    ]
}
