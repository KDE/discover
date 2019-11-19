import QtQuick.Controls 2.3
import QtQuick.Layouts 1.1
import QtQuick 2.4
import org.kde.discover 2.0
import org.kde.discover.app 1.0
import "navigation.js" as Navigation
import org.kde.kirigami 2.10 as Kirigami

DiscoverPage
{
    id: page
    title: i18n("Updates")

    property string footerLabel: ""
    property int footerProgress: 0
    property bool isBusy: false

    readonly property var resourcesUpdatesModel: ResourcesUpdatesModel {
        id: resourcesUpdatesModel
        onPassiveMessage: {
            desc.text += message + "<br/>\n"
            sheet.sheetOpen = true
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
        ColumnLayout {
            Label {
                id: desc
                Layout.fillWidth: true
                textFormat: Text.StyledText
                wrapMode: Text.WordWrap
            }

            Button {
                id: okButton
                Layout.alignment: Qt.AlignRight
                text: i18n("Proceed")
                icon.name: "dialog-ok"
                onClicked: {
                    sheet.sheetOpen = false
                }
            }
        }

        onSheetOpenChanged: if(!sheetOpen) {
            desc.text = ""
        } else {
            okButton.focus = true
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
        enabled: !resourcesUpdatesModel.isProgressing && !ResourcesModel.isFetching
        onTriggered: resourcesUpdatesModel.updateAll()
    }

    footer: ScrollView {
        id: scv
        width: parent.width
        height: visible ? Kirigami.Units.gridUnit * 10 : 0
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

    Kirigami.Action
    {
        id: cancelUpdateAction
        iconName: "dialog-cancel"
        text: i18n("Cancel")
        enabled: resourcesUpdatesModel.transaction && resourcesUpdatesModel.transaction.isCancellable
        onTriggered: resourcesUpdatesModel.transaction.cancel()
    }

    readonly property int unselected: (updateModel.totalUpdatesCount - updateModel.toUpdateCount)

    header: ToolBar {
        Kirigami.Theme.colorSet: Kirigami.Theme.Button
        Kirigami.Theme.inherit: false
        visible: (updateModel.totalUpdatesCount > 0 && resourcesUpdatesModel.isProgressing) || updateModel.hasUpdates

        RowLayout {
            anchors.fill: parent
            enabled: updateAction.enabled

            CheckBox {
                Layout.leftMargin: Kirigami.Units.gridUnit + Kirigami.Units.largeSpacing
                enabled: !resourcesUpdatesModel.isProgressing && !ResourcesModel.isFetching
                tristate: true
                checkState: updateModel.toUpdateCount === 0                             ? Qt.Unchecked
                          : updateModel.toUpdateCount === updateModel.totalUpdatesCount ? Qt.Checked
                                                                                        : Qt.PartiallyChecked

                onClicked: {
                    if (updateModel.toUpdateCount === 0)
                        updateModel.checkAll()
                    else
                        updateModel.uncheckAll()
                }
            }

            Label {
                Layout.fillWidth: true
                text: page.unselected === 0 ? i18n("All updates selected (%1)", updateModel.updateSize) : i18np("%1/%2 update selected (%3)", "%1/%2 updates selected (%3)", updateModel.toUpdateCount, updateModel.totalUpdatesCount, updateModel.updateSize)
                elide: Text.ElideRight
            }
        }
    }

    supportsRefreshing: true
    onRefreshingChanged: {
        ResourcesModel.updateAction.triggered()
        refreshing = false
    }

    readonly property Item report: ColumnLayout {
        parent: page
        anchors.fill: parent
        Item {
            Layout.fillHeight: true
            width: 1
        }
        Kirigami.Heading {
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignHCenter
            horizontalAlignment: Text.AlignHCenter
            text: page.footerLabel
            level: 3
        }
        ProgressBar {
            Layout.alignment: Qt.AlignHCenter
            Layout.preferredWidth: Kirigami.Units.gridUnit * 20
            value: page.footerProgress
            from: 0
            to: 100
            visible: page.isBusy
        }
        Kirigami.Icon {
            Layout.alignment: Qt.AlignHCenter
            visible: page.footerProgress === 0 && page.footerLabel !== "" && !page.isBusy
            source: "update-none"
            opacity: 0.1
            width: Kirigami.Units.gridUnit * 8
            height: width
        }
        Button {
            Layout.alignment: Qt.AlignHCenter
            text: i18n("Restart")
            visible: resourcesUpdatesModel.needsReboot
            onClicked: app.reboot()
        }
        Item {
            Layout.fillHeight: true
            width: 1
        }
    }
    ListView {
        id: updatesView
        currentIndex: -1

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
            backgroundColor: Kirigami.Theme.backgroundColor
            highlighted: ListView.isCurrentItem
            onEnabledChanged: if (!enabled) {
                layout.extended = false;
            }

            visible: resourceState < 3 //3=AbstractBackendUpdater.Done

            Keys.onReturnPressed: {
                itemChecked.clicked()
            }
            Keys.onPressed: if (event.key===Qt.Key_Alt) layout.extended = true
            Keys.onReleased: if (event.key===Qt.Key_Alt)  layout.extended = false

            ColumnLayout {
                id: layout
                property bool extended: false
                onExtendedChanged: if (extended) {
                    updateModel.fetchUpdateDetails(index)
                }
                RowLayout {
                    Layout.fillWidth: true
                    Layout.fillHeight: true

                    CheckBox {
                        id: itemChecked
                        Layout.leftMargin: Kirigami.Units.gridUnit
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
                            elide: Text.ElideRight
                            text: {
                                if (installedVersion == availableVersion && !truncated) {
                                    // Update of the same version; show when old and new are
                                    // the same (common with Flatpak runtimes)
                                    return i18n("Update to version %1", availableVersion);
                                } else if (installedVersion && availableVersion && !truncated) {
                                    // Old and new version numbers; show when there's enough room
                                    return i18n("%1 → %2", installedVersion, availableVersion);
                                } else {
                                    // Available version only, for when the installed version
                                    // isn't available for some reason, or when old+new would be
                                    // elided. Use squeezey text to gain more room, and if it's
                                    // still so long that it would be elided, elide from the left
                                    // so the most important part on the right is still visible

                                    // All of this is mostly for the benefit of KDE Neon users,
                                    // since the version strings there are really really long
                                    return availableVersion
                                }
                            }
                            font.letterSpacing: truncated ? -0.5 : 0
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

                Button {
                    Layout.alignment: Qt.AlignRight
                    text: i18n("More Information...")
                    visible: layout.extended
                    enabled: !resourcesUpdatesModel.isProgressing
                    onClicked: Navigation.openApplication(resource)
                }
            }

            onClicked: {
                layout.extended = !layout.extended
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
            PropertyChanges { target: page; footerLabel: i18nc("@info", "Fetching Updates...") }
            PropertyChanges { target: page; footerProgress: ResourcesModel.fetchingUpdatesProgress }
            PropertyChanges { target: page; isBusy: true }
            PropertyChanges { target: updatesView; opacity: 0 }
        },
        State {
            name: "progressing"
            PropertyChanges { target: page; supportsRefreshing: false }
            PropertyChanges { target: page.actions; main: cancelUpdateAction }
        },
        State {
            name: "has-updates"
            PropertyChanges { target: page; title: i18nc("@info", "Updates") }
            PropertyChanges { target: page.actions; main: updateAction }
            PropertyChanges { target: page.actions; left: refreshAction }
        },
        State {
            name: "reboot"
            PropertyChanges { target: page; footerLabel: i18nc("@info", "The system requires a restart to apply updates") }
        },
        State {
            name: "now-uptodate"
            PropertyChanges { target: page; footerLabel: i18nc("@info", "Up to Date") }
            PropertyChanges { target: page.actions; main: refreshAction }
        },
        State {
            name: "uptodate"
            PropertyChanges { target: page; footerLabel: i18nc("@info", "Up to Date") }
            PropertyChanges { target: page.actions; main: refreshAction }
        },
        State {
            name: "medium"
            PropertyChanges { target: page; title: i18nc("@info", "Up to Date") }
            PropertyChanges { target: page.actions; main: refreshAction }
        },
        State {
            name: "low"
            PropertyChanges { target: page; title: i18nc("@info", "Should check for updates") }
            PropertyChanges { target: page.actions; main: refreshAction }
        },
        State {
            name: "unknown"
            PropertyChanges { target: page; title: i18nc("@info", "It is unknown when the last check for updates was") }
            PropertyChanges { target: page.actions; main: refreshAction }
        }
    ]
}
