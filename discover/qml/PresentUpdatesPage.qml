import QtQuick.Controls 1.2
import QtQuick.Layouts 1.1
import QtQuick 2.1
import org.kde.discover 1.0
import org.kde.discover.app 1.0
import org.kde.kquickcontrolsaddons 2.0
import "navigation.js" as Navigation
import org.kde.kirigami 1.0 as Kirigami

ScrollView
{
    id: page

    function start() {
        resourcesUpdatesModel.prepare()
        resourcesUpdatesModel.updateAll()
    }

    ListView
    {
        header: PageHeader {
            width: parent.width
            background: "https://c2.staticflickr.com/4/3095/3246726097_711731f31a_b.jpg"
            ConditionalLoader {
                Layout.fillWidth: true

                condition: resourcesUpdatesModel.isProgressing
                onConditionChanged: {
                    window.navigationEnabled = !condition;
                }
                componentFalse: RowLayout {
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
                height: 1.5*implicitHeight
                horizontalAlignment: Text.AlignRight
                verticalAlignment: Text.AlignVCenter
                text: section
            }
        }

        spacing: Kirigami.Units.smallSpacing

        delegate: Kirigami.AbstractListItem {
            width: ListView.view.width

            ColumnLayout {
                id: layout
                anchors {
                    left: parent.left
                    right: parent.right
                }
                enabled: !resourcesUpdatesModel.isProgressing
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
