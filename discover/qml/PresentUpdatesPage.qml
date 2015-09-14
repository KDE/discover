import QtQuick.Controls 1.1
import QtQuick.Layouts 1.1
import QtQuick 2.1
import org.kde.muon 1.0
import org.kde.kquickcontrolsaddons 2.0

ScrollView
{
    id: page
    readonly property real proposedMargin: (width-app.actualWidth)/2
    readonly property Component tools: RowLayout {
        Button {
            text: i18n("Update")
            enabled: updateModel.hasUpdates
            onClicked: {
                var updates = page.Stack.view.push(updatesPage)
                updates.start()
            }
        }
    }

    Component {
        id: updatesPage
        UpdatesPage {}
    }

    ColumnLayout
    {
        x: proposedMargin
        width: app.actualWidth
        Repeater {
            id: rep
            model: UpdateModel {
                id: updateModel
                backend: ResourcesUpdatesModel {
                    id: updates
                }
            }
            delegate: ColumnLayout {
                id: col
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
                            anchors.fill: parent
                            spacing: 5
                            CheckBox {
                                anchors.verticalCenter: parent.verticalCenter
                                checked: model.checked
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
                                text: i18n("%1 (%2) - %3", display, version, size)
                                elide: Text.ElideRight
                            }
                        }
                    }
                }
            }
        }
    }
}
