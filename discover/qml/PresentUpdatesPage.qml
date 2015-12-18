import QtQuick.Controls 1.2
import QtQuick.Layouts 1.1
import QtQuick 2.1
import org.kde.discover 1.0
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

    ColumnLayout
    {
        x: proposedMargin
        width: Math.min(app.actualWidth, page.viewport.width)

        PageHeader {
            Layout.fillWidth: true

            ConditionalLoader {
                anchors.fill: parent

                condition: resourcesUpdatesModel.isProgressing
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

        Repeater {
            id: rep
            model: updateModel

            delegate: ColumnLayout {
                id: col
                spacing: -1
                readonly property var currentRow: index
                RowLayout {
                    Layout.minimumHeight: 32
                    Layout.leftMargin: 5 //GridItem.internalMargin
                    Layout.rightMargin: 5 //GridItem.internalMargin
                    anchors.margins: 100
                    Label {
                        Layout.fillWidth: true
                        text: display
                    }
                    LabelBackground {
                        text: size
                        Layout.minimumWidth: 90
                    }
                }
                Repeater {
                    model: ColumnProxyModel {
                        rootIndex: updateModel.index(col.currentRow, 0)
                    }
                    delegate: GridItem {
                        Layout.fillWidth: true
                        height: 32
                        RowLayout {
                            enabled: !resourcesUpdatesModel.isProgressing
                            anchors.fill: parent
                            CheckBox {
                                anchors.verticalCenter: parent.verticalCenter
                                checked: model.checked
                                onClicked: model.checked = !model.checked
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
                            }
                        }

                        onClicked: Navigation.openApplication(resource)
                    }
                }
            }
        }
    }
}
