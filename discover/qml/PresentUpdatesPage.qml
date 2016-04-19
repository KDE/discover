import QtQuick.Controls 1.2
import QtQuick.Layouts 1.1
import QtQuick 2.1
import org.kde.discover 1.0
import org.kde.discover.app 1.0
import "navigation.js" as Navigation
import org.kde.kquickcontrolsaddons 2.0

ScrollView
{
    id: page
    property real proposedMargin: 0

    function start() {
        resourcesUpdatesModel.prepare()
        resourcesUpdatesModel.updateAll()
    }

    ListView
    {
        header: PageHeader {
            x: proposedMargin
            width: Math.min(Helpers.actualWidth, page.viewport.width)

            ConditionalLoader {
                anchors.fill: parent

                condition: resourcesUpdatesModel.isProgressing
                onConditionChanged: {
                    window.navigationEnabled = !condition;
                }
                componentFalse: RowLayout {
                    LabelBackground {
                        text: updateModel.toUpdateCount
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
                        text: i18n("Update")
                        onClicked: page.start()
                    }
                }
                componentTrue: ColumnLayout {
                    Label {
                        Layout.fillWidth: true
                        horizontalAlignment: Text.AlignHCenter
                        text: resourcesUpdatesModel.remainingTime
                    }
                    ProgressBar {
                        id: pbar
                        anchors.centerIn: parent
                        minimumValue: 0
                        maximumValue: 100

                        // Workaround for bug in Qt
                        // https://bugreports.qt.io/browse/QTBUG-48598
                        Connections {
                            target: resourcesUpdatesModel
                            onProgressChanged: pbar.value = resourcesUpdatesModel.progress
                        }
                    }
                }
            }
        }

        footer: Item { width: 5; height: 5 }

        model: updateModel

        section {
            property: "section"
            delegate: Label {
                x: proposedMargin
                width: Math.min(Helpers.actualWidth, page.viewport.width)
                height: 1.5*implicitHeight
                horizontalAlignment: Text.AlignRight
                verticalAlignment: Text.AlignVCenter
                text: section
            }
        }

        delegate: GridItem {
            x: proposedMargin
            width: Math.min(Helpers.actualWidth, page.viewport.width)
            height: layout.extended ? 200 : layout.implicitHeight + 2*internalMargin

            ColumnLayout {
                id: layout
                enabled: !resourcesUpdatesModel.isProgressing
                property bool extended: false
                anchors.fill: parent
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
                        id: label
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
                    visible: layout.extended && changelog !== ""

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

            onClicked: layout.extended = !layout.extended
        }
    }
}
