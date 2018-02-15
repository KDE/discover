import QtQuick 2.1
import QtQuick.Controls 1.1
import QtQuick.Controls 2.1 as QQC2
import QtQuick.Layouts 1.1
import org.kde.kquickcontrolsaddons 2.0
import org.kde.discover 2.0
import org.kde.kirigami 2.0 as Kirigami
import "navigation.js" as Navigation

Kirigami.BasicListItem {
    id: listItem
    label: TransactionModel.count ? i18n("Tasks (%1%)", TransactionModel.progress) : i18n("Tasks")
    visible: progressModel.count > 0

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
    onVisibleChanged: if (!visible) {
        sheetObject.close()
        sheetObject.destroy(100)
    }

    readonly property var v1: Connections {
        target: TransactionModel
        onTransactionAdded: {
            if(listItem.enabled && trans.visible && progressModel.applicationAt(trans.resource)<0) {
                progressModel.append({ transaction: trans })
            }
        }

        onTransactionRemoved: {
            if (!trans.resource) {
                var id = progressModel.applicationAt(trans.resource)
                if(id>=0)
                    progressModel.remove(id)
            }
        }
    }
    
    readonly property var v2: ListModel {
        id: progressModel
        function applicationAt(app) {
            for(var i=0; i<progressModel.count; i++) {
                if(progressModel.get(i).application==app) {
                    return i
                }
            }
            return -1
        }
    }

    readonly property var v3: Component {
        id: sheet
        Kirigami.OverlaySheet {

            contentItem: ColumnLayout {
                spacing: 0

                Component {
                    id: listenerComp
                    TransactionListener {
                        property int index: -1
                        onCancelled: {
                            progressModel.remove(index)
                        }
                    }
                }

                Repeater {
                    model: progressModel

                    delegate: Kirigami.AbstractListItem {
                        id: del
                        separatorVisible: false
                        onClicked: {
                            if (model.application) {
                                Navigation.clearStack()
                                Navigation.openApplication(model.application)
                            }
                        }
                        readonly property QtObject listener: listenerComp.createObject(del, (model.transaction.resource ? {resource: model.transaction.resource, index: index} : {transaction: model.transaction, index: index}))

                        ColumnLayout {
                            width: parent.width

                            RowLayout {
                                Layout.fillWidth: true

                                Kirigami.Icon {
                                    Layout.fillHeight: true
                                    Layout.minimumWidth: height
                                    source: model.transaction.icon
                                }

                                QQC2.Label {
                                    anchors.verticalCenter: parent.verticalCenter
                                    Layout.fillWidth: true
                                    elide: Text.ElideRight
                                    text: listener.isActive ? i18nc("TransactioName TransactionStatus", "%1 %2", model.transaction.name, listener.statusText) : model.transaction.name
                                }
                                ToolButton {
                                    iconName: "dialog-cancel"
                                    visible: listener.isCancellable
                                    onClicked: listener.cancel()
                                }
                                ToolButton {
                                    iconName: "system-run"
                                    visible: model.application != undefined && model.application.isInstalled && !listener.isActive && model.application.canExecute
                                    onClicked: {
                                        model.application.invokeApplication()
                                        model.remove(index)
                                    }
                                }
                            }
                            ProgressBar {
                                Layout.fillWidth: true
                                visible: listener.isActive
                                value: listener.progress
                                maximumValue: 100
                            }
                        }
                    }
                }
            }
        }
    }
}
