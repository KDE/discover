import QtQuick.Controls 1.1
import QtQuick.Layouts 1.1
import QtQuick 2.1
import org.kde.muon 1.0
import org.kde.kquickcontrolsaddons 2.0

ScrollView
{
    id: page
    property real proposedMargin: 0

    ColumnLayout
    {
        x: proposedMargin
        width: app.actualWidth
        Repeater {
            id: rep
            model: updateModel

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
