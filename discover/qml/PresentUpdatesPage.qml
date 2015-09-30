import QtQuick.Controls 1.1
import QtQuick.Layouts 1.1
import QtQuick 2.1
import org.kde.muon 1.0
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
        width: app.actualWidth

        GridItem {
            Layout.fillWidth: true
            height: 50
            hoverEnabled: false

            ConditionalLoader {
                anchors {
                    left: parent.left
                    right: parent.right
                    verticalCenter: parent.verticalCenter
                    margins: 10
                }

                condition: resourcesUpdatesModel.isProgressing
                componentFalse: RowLayout {
                    LabelBackground {
                        anchors.verticalCenter: parent.verticalCenter
                        text: updateModel.toUpdateCount
                    }
                    Label {
                        anchors.verticalCenter: parent.verticalCenter
                        text: i18n("updates selected")
                    }
                    LabelBackground {
                        anchors.verticalCenter: parent.verticalCenter
                        text: (updateModel.totalUpdatesCount - updateModel.toUpdateCount)
                    }
                    Label {
                        anchors.verticalCenter: parent.verticalCenter
                        text: i18n("updates not selected")
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
                        text: resourcesUpdatesModel.remainingTime
                    }
                    ProgressBar {
                        anchors.centerIn: parent
                        value: resourcesUpdatesModel.progress
                    }
                }
            }
        }

        Repeater {
            id: rep
            model: updateModel

            delegate: ColumnLayout {
                id: col
                spacing: -2
                property var currentRow: index
                Label {
                    Layout.fillWidth: true
                    horizontalAlignment: Text.AlignRight
                    text: display
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
                                text: size
                            }
                        }
                    }
                }
            }
        }
    }
}
