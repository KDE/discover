import QtQuick 2.1
import QtQuick.Controls 1.1
import QtQuick.Layouts 1.1
import org.kde.kquickcontrolsaddons 2.0
import org.kde.discover 1.0
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

    onClicked: {
        sheet.open()
    }

    readonly property var v1: Connections {
        target: TransactionModel
        onTransactionAdded: {
            if(listItem.enabled && trans.resource && progressModel.appAt(trans.resource)<0)
                progressModel.append({app: trans.resource})
        }

        onTransactionCancelled: {
            var id = progressModel.appAt(trans.resource)
            if(id>=0)
                progressModel.remove(id)
        }
    }
    
    readonly property var v2: ListModel {
        id: progressModel
        function appAt(app) {
            for(var i=0; i<progressModel.count; i++) {
                if(progressModel.get(i).app==app) {
                    return i
                }
            }
            return -1
        }
    }
    
    readonly property var v3: Kirigami.OverlaySheet {
        id: sheet

        contentItem: ColumnLayout {
            spacing: 0
            Repeater {
                model: progressModel

                delegate: Kirigami.AbstractListItem {
                    separatorVisible: false
                    onClicked: {
                        Navigation.clearStack()
                        Navigation.openApplication(model.app)
                    }

                    ColumnLayout {
                        width: parent.width
                        TransactionListener {
                            id: listener
                            resource: model.app
                            onCancelled: progressModel.remove(index)
                        }

                        RowLayout {
                            Layout.fillWidth: true

                            Kirigami.Icon {
                                Layout.fillHeight: true
                                Layout.minimumWidth: height
                                source: model.app.icon
                            }
                            Label {
                                anchors.verticalCenter: parent.verticalCenter
                                Layout.fillWidth: true
                                elide: Text.ElideRight
                                text: model.app.name + (listener.isActive ? " "+listener.statusText : "")
                            }
                            ToolButton {
                                iconName: "dialog-cancel"
                                visible: listener.isCancellable
                                onClicked: listener.cancel()
                            }
                            ToolButton {
                                iconName: "system-run"
                                visible: model.app.isInstalled && !listener.isActive && model.app.canExecute
                                onClicked: {
                                    model.app.invokeApplication()
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
