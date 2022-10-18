import QtQuick.Controls 2.3
import QtQuick.Layouts 1.1
import QtQuick 2.15
import org.kde.discover 2.0
import org.kde.discover.app 1.0
import "navigation.js" as Navigation
import org.kde.kirigami 2.19 as Kirigami

DiscoverPage
{
    id: page
    title: i18n("Updates")

    property string footerLabel: ""
    property int footerProgress: 0
    property bool isBusy: false
    readonly property string name: title

    readonly property var resourcesUpdatesModel: ResourcesUpdatesModel {
        id: resourcesUpdatesModel
        onPassiveMessage: {
            sheet.errorMessage = message;
            sheet.sheetOpen = true;
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

    readonly property var sheet: Kirigami.OverlaySheet {
        id: sheet

        property string errorMessage: ""

        parent: applicationWindow().overlay

        title: contentLoader.sourceComponent === friendlyMessageComponent ? i18n("Update Issue") :  i18n("Technical details")

        Loader {
            id: contentLoader
            active: true
            sourceComponent: friendlyMessageComponent

            Component {
                id: friendlyMessageComponent

                ColumnLayout {
                    Label {
                        id: friendlyMessage
                        Layout.fillWidth: true
                        Layout.maximumWidth: Math.round(page.width*0.75)
                        Layout.bottomMargin: Kirigami.Units.largeSpacing * 2
                        text: i18n("There was an issue installing this update. Please try again later.")
                        wrapMode: Text.WordWrap
                    }
                    Button {
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
                    Label {
                        Layout.fillWidth: true
                        Layout.maximumWidth: Math.round(page.width*0.75)
                        text: i18n("If you would like to report the update issue to your distribution's packagers, include this information:")
                        wrapMode: Text.WordWrap
                    }

                    TextArea {
                        Layout.fillWidth: true
                        text: sheet.errorMessage
                        textFormat: Text.RichText
                        wrapMode: Text.WordWrap
                    }

                    RowLayout {
                        Layout.alignment: Qt.AlignRight

                        Button {
                            text: i18n("Copy to Clipboard")
                            icon.name: "edit-copy"
                            onClicked: {
                                app.copyTextToClipboard(sheet.errorMessage);
                                window.showPassiveNotification(i18n("Error message copied to clipboard"));
                            }
                        }

                        Button {
                            text: i18n("Report This Issue")
                            icon.name: "tools-report-bug"
                            onClicked: {
                                Qt.openUrlExternally(ResourcesModel.distroBugReportUrl())
                                sheet.sheetOpen = false
                            }
                        }
                    }
                }
            }
        }

        // Ensure that friendly message is shown if the user closes the sheet and
        // then opens it again
        onSheetOpenChanged: if (sheetOpen) {
            contentLoader.sourceComponent = friendlyMessageComponent;
        }
    }

    readonly property var updateModel: UpdateModel {
        id: updateModel
        backend: resourcesUpdatesModel
    }

    readonly property var updateAction: Kirigami.Action
    {
        id: updateAction
        text: page.unselected>0 ? i18n("Update Selected") : i18n("Update All")
        visible: updateModel.toUpdateCount
        iconName: "update-none"

        function anyVisible(items) {
            for (const itemPos in items) {
                const item = items[itemPos];
                if (item.visible && item instanceof Kirigami.InlineMessage) {
                    return true
                }
            }
            return false;
        }

        enabled: !resourcesUpdatesModel.isProgressing && !ResourcesModel.isFetching && !anyVisible(page.header.children)
        onTriggered: resourcesUpdatesModel.updateAll()
    }

    header: ColumnLayout {
        id: errorsColumn
        Loader {
            Layout.fillWidth: true
            Layout.margins: Kirigami.Units.smallSpacing
            active: ResourcesModel.inlineMessage
            sourceComponent: Kirigami.InlineMessage {
                text: ResourcesModel.inlineMessage.message
                type: ResourcesModel.inlineMessage.type
                icon.name: ResourcesModel.inlineMessage.iconName
                visible: true
            }
        }

        Repeater {
            model: resourcesUpdatesModel.errorMessages
            delegate: Kirigami.InlineMessage {
                id: inline
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

        ScrollView {
            id: scv
            Layout.fillWidth: true
            Layout.preferredHeight: visible ? Kirigami.Units.gridUnit * 10 : 0
            visible: log.contents.length > 0
            TextArea {
                readOnly: true
                text: log.contents

                cursorPosition: text.length - 1
                font.family: "monospace"

                ReadFile {
                    id: log
                    filter: ".*ALPM-SCRIPTLET\\] .*"
                    path: "/var/log/pacman.log"
                }
            }
        }

        ToolBar {
            id: footerToolbar
            Layout.fillWidth: true
            visible: (updateModel.totalUpdatesCount > 0 && resourcesUpdatesModel.isProgressing) || updateModel.hasUpdates

            position: ToolBar.Footer

            contentItem: RowLayout {
                ToolButton {
                    enabled: page.unselected > 0 && updateAction.enabled && !resourcesUpdatesModel.isProgressing && !ResourcesModel.isFetching
                    visible: updateModel.totalUpdatesCount > 1 && !resourcesUpdatesModel.needsReboot
                    icon.name: "edit-select-all"
                    text: i18n("Select All")
                    onClicked: { updateModel.checkAll(); }
                }

                ToolButton {
                    enabled: page.unselected !== updateModel.totalUpdatesCount && updateAction.enabled && !resourcesUpdatesModel.isProgressing && !ResourcesModel.isFetching
                    visible: updateModel.totalUpdatesCount > 1 && !resourcesUpdatesModel.needsReboot
                    icon.name: "edit-select-none"
                    text: i18n("Select None")
                    onClicked: { updateModel.uncheckAll(); }
                }

                CheckBox {
                    id: rebootAtEnd
                    visible: resourcesUpdatesModel.needsReboot
                    text: i18n("Restart automatically after update has completed");
                }

                Label {
                    Layout.fillWidth: true
                    Layout.rightMargin: Kirigami.Units.largeSpacing
                    horizontalAlignment: Text.AlignRight
                    text: i18n("Total size: %1", updateModel.updateSize)
                    elide: Text.ElideLeft
                }
            }
        }
    }

    Kirigami.Action
    {
        id: cancelUpdateAction
        iconName: "dialog-cancel"
        text: i18n("Cancel")
        enabled: resourcesUpdatesModel.transaction && resourcesUpdatesModel.transaction.isCancellable
        onTriggered: resourcesUpdatesModel.transaction.cancel()
    }

    readonly property int unselected: (updateModel.totalUpdatesCount - updateModel.toUpdateCount)

    supportsRefreshing: true
    onRefreshingChanged: {
        ResourcesModel.updateAction.triggered()
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
                if (page.footerProgress === 0 && page.footerLabel !== "" && !page.isBusy) {
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

        displaced: Transition {
            YAnimator {
                duration: Kirigami.Units.longDuration
                easing.type: Easing.InOutQuad
            }
        }

        model: QSortFilterProxyModel {
            sourceModel: updateModel
            sortRole: UpdateModel.SectionResourceProgressRole
        }

        section {
            property: "section"
            delegate: Kirigami.ListSectionHeader {
                width: updatesView.width
                label: section
            }
        }

        delegate: Kirigami.AbstractListItem {
            id: listItem
            highlighted: ListView.isCurrentItem
            hoverEnabled: !page.isBusy
            onEnabledChanged: if (!enabled) {
                model.extended = false;
            }

            visible: resourceState < 3 //3=AbstractBackendUpdater.Done

            Keys.onReturnPressed: {
                itemChecked.clicked()
            }
            Keys.onPressed: if (event.key===Qt.Key_Alt) model.extended = true
            Keys.onReleased: if (event.key===Qt.Key_Alt) model.extended = false

            ColumnLayout {
                id: layout
                property bool extended: model.extended
                onExtendedChanged: if (extended) {
                    updateModel.fetchUpdateDetails(index)
                }
                RowLayout {
                    Layout.fillWidth: true
                    Layout.fillHeight: true

                    CheckBox {
                        id: itemChecked
                        Layout.alignment: Qt.AlignVCenter
                        checked: model.checked === Qt.Checked
                        onClicked: model.checked = (model.checked===Qt.Checked ? Qt.Unchecked : Qt.Checked)
                        enabled: !resourcesUpdatesModel.isProgressing
                    }

                    Kirigami.Icon {
                        width: Kirigami.Units.gridUnit * 2
                        Layout.preferredHeight: width
                        source: decoration
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
                            text: i18n("%1", display)
                            level: 3
                            elide: Text.ElideRight
                        }

                        // Version numbers
                        Label {
                            Layout.fillWidth: true
                            elide: truncated ? Text.ElideLeft : Text.ElideRight
                            text: resource.upgradeText
                            opacity: listItem.hovered? 0.8 : 0.6
                        }
                    }

                    LabelBackground {
                        Layout.minimumWidth: Kirigami.Units.gridUnit * 6
                        text: resourceState == 2 ? i18n("Installing") : size

                        progress: resourceProgress/100
                    }
                }

                Frame {
                    Layout.fillWidth: true
                    implicitHeight: view.contentHeight
                    visible: layout.extended && changelog.length>0
                    Label {
                        id: view
                        anchors {
                            right: parent.right
                            left: parent.left
                        }
                        text: changelog
                        textFormat: Text.StyledText
                        wrapMode: Text.WordWrap
                        onLinkActivated: Qt.openUrlExternally(link)

                    }

                    //This saves a binding loop on implictHeight, as the Label
                    //height is updated twice (first time with the wrong value)
                    Behavior on implicitHeight
                    { PropertyAnimation { duration: Kirigami.Units.shortDuration } }
                }

                RowLayout {
                    Layout.fillWidth: true
                    visible: layout.extended

                    Label {
                        Layout.leftMargin: Kirigami.Units.gridUnit
                        text: i18n("Update from:")
                    }
                    // Backend icon
                    Kirigami.Icon {
                        source: resource.sourceIcon
                        implicitWidth: Kirigami.Units.iconSizes.smallMedium
                        implicitHeight: Kirigami.Units.iconSizes.smallMedium
                    }
                    // Backend label and origin/remote
                    Label {
                        Layout.fillWidth: true
                        text: resource.origin.length === 0 ? resource.backend.displayName
                                : i18nc("%1 is the backend that provides this app, %2 is the specific repository or address within that backend","%1 (%2)",
                                        resource.backend.displayName, resource.origin)
                        elide: Text.ElideRight
                    }

                    Button {
                        Layout.alignment: Qt.AlignRight
                        text: i18n("More Information…")
                        enabled: !resourcesUpdatesModel.isProgressing
                        onClicked: Navigation.openApplication(resource)
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
            : ResourcesModel.isFetching                  ? "fetching"
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
            PropertyChanges { target: page; footerProgress: ResourcesModel.fetchingUpdatesProgress }
            PropertyChanges { target: page; isBusy: true }
            PropertyChanges { target: updatesView; opacity: 0 }
        },
        State {
            name: "progressing"
            PropertyChanges { target: page; supportsRefreshing: false }
            PropertyChanges { target: page.actions; main: cancelUpdateAction }
            PropertyChanges { target: statusLabel; visible: false }
        },
        State {
            name: "has-updates"
            PropertyChanges { target: page; title: i18nc("@info", "Updates") }
            // On mobile, we want "Update" to be the primary action so it's in
            // the center, but on desktop this feels a bit awkward and it would
            // be better to have "Update" be the right-most action
            PropertyChanges { target: page.actions; main: applicationWindow().wideScreen ? refreshAction : updateAction}
            PropertyChanges { target: page.actions; left: applicationWindow().wideScreen ? updateAction : refreshAction}
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
            PropertyChanges { target: page.actions; main: refreshAction }
            PropertyChanges { target: statusLabel; explanation: "" }
            PropertyChanges { target: statusLabel.progressBar; visible: false }
        },
        State {
            name: "uptodate"
            PropertyChanges { target: page; footerLabel: i18nc("@info", "Up to date") }
            PropertyChanges { target: page.actions; main: refreshAction }
            PropertyChanges { target: statusLabel; explanation: "" }
            PropertyChanges { target: statusLabel.progressBar; visible: false }
        },
        State {
            name: "medium"
            PropertyChanges { target: page; title: i18nc("@info", "Up to date") }
            PropertyChanges { target: page.actions; main: refreshAction }
            PropertyChanges { target: statusLabel; explanation: "" }
            PropertyChanges { target: statusLabel.progressBar; visible: false }
        },
        State {
            name: "low"
            PropertyChanges { target: page; title: i18nc("@info", "Should check for updates") }
            PropertyChanges { target: page.actions; main: refreshAction }
            PropertyChanges { target: statusLabel; explanation: "" }
            PropertyChanges { target: statusLabel.progressBar; visible: false }
        },
        State {
            name: "unknown"
            PropertyChanges { target: page; title: i18nc("@info", "Time of last update unknown") }
            PropertyChanges { target: page.actions; main: refreshAction }
            PropertyChanges { target: statusLabel; explanation: "" }
            PropertyChanges { target: statusLabel.progressBar; visible: false }
        }
    ]
}
