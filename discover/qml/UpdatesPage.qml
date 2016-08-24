import QtQuick.Controls 1.2
import QtQuick.Layouts 1.1
import QtQuick 2.1
import org.kde.discover 1.0
import org.kde.discover.app 1.0
import org.kde.kquickcontrolsaddons 2.0
import org.kde.kcoreaddons 1.0
import "navigation.js" as Navigation
import org.kde.kirigami 1.0 as Kirigami

DiscoverPage
{
    id: page
    title: i18n("Updates")

    function start() {
        resourcesUpdatesModel.prepare()
        resourcesUpdatesModel.updateAll()
    }

    property string footerLabel: ""

    //TODO: use supportsRefreshing to fetch updates

    ListView
    {
        ResourcesUpdatesModel {
            id: resourcesUpdatesModel
            onIsProgressingChanged: {
                window.navigationEnabled = !isProgressing
            }
        }

        UpdateModel {
            id: updateModel
            backend: resourcesUpdatesModel
        }

        BusyIndicator {
            anchors.centerIn: parent
            visible: ResourcesModel.isFetching
            enabled: visible
            anchors.horizontalCenter: parent.horizontalCenter
            width: 32
            height: 32
        }

        header: PageHeader {
            id: header
            anchors {
                left: parent.left
                right: parent.right
            }
            background: "https://c2.staticflickr.com/4/3095/3246726097_711731f31a_b.jpg"

            RowLayout {
                Layout.fillWidth: true
                Layout.leftMargin: Kirigami.Units.gridUnit
                Layout.rightMargin: Kirigami.Units.gridUnit
                Layout.topMargin: Kirigami.Units.smallSpacing
                Layout.bottomMargin: Kirigami.Units.smallSpacing

                enabled: !resourcesUpdatesModel.isProgressing
                visible: resourcesUpdatesModel.isProgressing || updateModel.hasUpdates

                LabelBackground {
                    text: updateModel.toUpdateCount + " (" + updateModel.updateSize+")"
                }
                Label {
                    text: i18n("updates selected")
                }
                LabelBackground {
                    id: unselectedItem
                    readonly property int unselected: (updateModel.totalUpdatesCount - updateModel.toUpdateCount)
                    text: unselected
                    visible: unselected>0
                }
                Label {
                    text: i18n("updates not selected")
                    visible: unselectedItem.visible
                }
                Item { Layout.fillWidth: true}
                Button {
                    id: startButton
                    text: unselectedItem.visible ? i18n("Update Selected") : i18n("Update All")
                    onClicked: page.start()
                }
            }

            readonly property var secSinceUpdate: resourcesUpdatesModel.secsToLastUpdate
            state: ( ResourcesModel.isFetching                  ? "fetching"
                   : updateModel.hasUpdates                     ? "has-updates"
                   : secSinceUpdate < 0                         ? "unknown"
                   : secSinceUpdate === 0                       ? "now-uptodate"
                   : secSinceUpdate < 1000 * 60 * 60 * 24       ? "uptodate"
                   : secSinceUpdate < 1000 * 60 * 60 * 24 * 7   ? "medium"
                   :                                              "low"
                   )

                states: [
                    State {
                        name: "fetching"
                        PropertyChanges { target: page; title: i18nc("@info", "Loading...") }
                        PropertyChanges { target: header; background: "https://c2.staticflickr.com/4/3873/14950433815_1794b390d4_b.jpg" }
                    },
                    State {
                        name: "has-updates"
                        PropertyChanges { target: page; title: i18nc("@info", "Updates") }
                        PropertyChanges { target: header; background: "https://c2.staticflickr.com/4/3873/14950433815_1794b390d4_b.jpg" }
                    },
                    State {
                        name: "now-uptodate"
                        PropertyChanges { target: page; title: i18nc("@info", "The system is up to date.") }
                        PropertyChanges { target: header; background: "https://c2.staticflickr.com/4/3095/3246726097_711731f31a_b.jpg" }
                        PropertyChanges { target: page; footerLabel: i18nc("@info", "No updates") }
                    },
                    State {
                        name: "uptodate"
                        PropertyChanges { target: page; title: i18nc("@info", "The system is up to date.") }
                        PropertyChanges { target: header; background: "https://c2.staticflickr.com/4/3095/3246726097_711731f31a_b.jpg" }
                    },
                    State {
                        name: "medium"
                        PropertyChanges { target: page; title: i18nc("@info", "No updates are available.") }
                        PropertyChanges { target: header; background: "https://c2.staticflickr.com/4/3095/3246726097_711731f31a_b.jpg" }
                    },
                    State {
                        name: "low"
                        PropertyChanges { target: page; title: i18nc("@info", "Should check for updates.") }
                        PropertyChanges { target: header; background: "https://c2.staticflickr.com/4/3100/2466596520_776eda5d3d_o.jpg" }
                    },
                    State {
                        name: "unknown"
                        PropertyChanges { target: page; title: i18nc("@info", "It is unknown when the last check for updates was.") }
                        PropertyChanges { target: header; background: "https://c2.staticflickr.com/4/3100/2466596520_776eda5d3d_o.jpg" }
                    }
                ]
        }

        footer: ColumnLayout {
            anchors.horizontalCenter: parent.horizontalCenter
            Kirigami.Heading {
                Layout.alignment: Qt.AlignHCenter
                visible: page.footerLabel !== ""
                text: page.footerLabel
            }
            QIconItem {
                Layout.alignment: Qt.AlignHCenter
                visible: page.footerLabel !== ""
                icon: "update-none"
                opacity: 0.3
                width: 200
                height: 200
            }
            Item {
                visible: page.footerLabel === ""
                height: Kirigami.Units.gridUnit
                width: 1
            }
        }

        model: updateModel

        section {
            property: "section"
            delegate: Label {
                height: 1.5*implicitHeight
                horizontalAlignment: Text.AlignRight
                verticalAlignment: Text.AlignVCenter
                text: section
            }
        }

        spacing: Kirigami.Units.smallSpacing

        delegate: Kirigami.AbstractListItem {
            x: Kirigami.Units.gridUnit
            width: ListView.view.width - Kirigami.Units.gridUnit * 2
            enabled: !resourcesUpdatesModel.isProgressing
            onEnabledChanged: if (!enabled) {
                layout.extended = false;
            }

            ColumnLayout {
                id: layout
                anchors {
                    left: parent.left
                    right: parent.right
                }
                property bool extended: false
                RowLayout {
                    Layout.fillWidth: true
                    CheckBox {
                        anchors.verticalCenter: parent.verticalCenter
                        checked: model.checked == Qt.Checked
                        onClicked: model.checked = (model.checked==Qt.Checked ? Qt.Unchecked : Qt.Checked)
                    }

                    QIconItem {
                        Layout.fillHeight: true
                        anchors.verticalCenter: parent.verticalCenter
                        width: 30
                        icon: decoration
                    }

                    Label {
                        Layout.fillWidth: true
                        text: i18n("%1 (%2)", display, version)
                        elide: Text.ElideRight
                    }

                    LabelBackground {
                        Layout.minimumWidth: 90
                        text: size

                        progressing: resourcesUpdatesModel.isProgressing
                        progress: resourceProgress/100
                    }
                }

                ScrollView {
                    id: view
                    Layout.fillHeight: true
                    Layout.fillWidth: true
                    frameVisible: true
                    visible: layout.extended && changelog.length>0

                    Label {
                        width: view.width-32
                        text: changelog
                        textFormat: Text.RichText
                        wrapMode: Text.WordWrap
                    }
                }

                Button {
                    text: i18n("Open")
                    visible: layout.extended
                    onClicked: Navigation.openApplication(resource)
                }
            }

            onClicked: {
                updateModel.fetchChangelog(index)
                layout.extended = !layout.extended
            }
        }
    }
}
