import QtQuick 2.1
import QtQuick.Controls 2.1
import QtQuick.Layouts 1.1
import org.kde.discover 2.0
import org.kde.kirigami 2.0 as Kirigami
import "navigation.js" as Navigation

Kirigami.BasicListItem {
    id: listItem
    label: TransactionModel.count ? i18n("Tasks (%1%)", TransactionModel.progress) : i18n("Tasks")
    visible: TransactionModel.count > 0

    background: Item {

        Rectangle {
            anchors {
                fill: parent
                rightMargin: TransactionModel.count>=1 ? listItem.width*(1-TransactionModel.progress/100) : 0
            }
            color: TransactionModel.count>=1 || listItem.hovered || listItem.highlighted || listItem.pressed || listItem.checked ? listItem.activeBackgroundColor : listItem.backgroundColor
            opacity: listItem.hovered || listItem.highlighted ? 0.2 : 1
        }
    }

    property QtObject sheetObject: null
    onClicked: {
        sheetObject = sheet.createObject()
        sheetObject.open()
    }
    onVisibleChanged: if (!visible && sheetObject) {
        sheetObject.close()
        sheetObject.destroy(100)
    }

    readonly property var v3: Component {
        id: sheet
        Kirigami.OverlaySheet {

            contentItem: ListView {
                spacing: 0

                Component {
                    id: listenerComp
                    TransactionListener {}
                }
                model: TransactionModel

                delegate: Kirigami.AbstractListItem {
                    id: del
                    separatorVisible: false
                    hoverEnabled: model.application
                    onClicked: {
                        if (model.application) {
                            Navigation.clearStack()
                            Navigation.openApplication(model.application)
                        }
                    }
                    readonly property QtObject listener: listenerComp.createObject(del, (model.transaction.resource ? {resource: model.transaction.resource} : {transaction: model.transaction}))

                    ColumnLayout {
                        width: parent.width

                        RowLayout {
                            Layout.fillWidth: true

                            Kirigami.Icon {
                                Layout.fillHeight: true
                                Layout.minimumWidth: height
                                source: model.transaction.icon
                            }

                            Label {
                                Layout.alignment: Qt.AlignVCenter
                                Layout.fillWidth: true
                                elide: Text.ElideRight
                                text: listener.isActive && model.transaction.downloadSpeed>0 ? i18nc("TransactioName - TransactionStatus", "%1 - %2: %3", model.transaction.name, listener.statusText, model.transaction.downloadSpeedString) :
                                                                           listener.isActive ? i18nc("TransactioName - TransactionStatus", "%1 - %2", model.transaction.name, listener.statusText)
                                                                                             : model.transaction.name
                            }
                            ToolButton {
                                icon.name: "dialog-cancel"
                                visible: listener.isCancellable
                                onClicked: listener.cancel()
                            }
                            ToolButton {
                                icon.name: "system-run"
                                visible: model.application !== undefined && model.application.isInstalled && !listener.isActive && model.application.canExecute
                                onClicked: {
                                    model.application.invokeApplication()
                                    model.remove(index)
                                }
                            }
                        }
                        ProgressBar {
                            Layout.fillWidth: true
                            visible: listener.isActive
                            value: listener.progress / 100
                        }
                    }
                }
            }
        }
    }
}
