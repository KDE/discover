import QtQuick 2.1
import QtQuick.Controls 1.1
import QtQuick.Layouts 1.1
import org.kde.discover 1.0
import org.kde.kquickcontrolsaddons 2.0

ColumnLayout
{
    id: addonsView
    property alias application: addonsModel.application
    property bool isInstalling: false
    property alias isEmpty: addonsModel.isEmpty
    enabled: !addonsView.isInstalling
    visible: !addonsView.isEmpty
    spacing: 5

    Repeater
    {
        model: ApplicationAddonsModel { id: addonsModel }
        
        delegate: RowLayout {
            Layout.fillWidth: true

            CheckBox {
                enabled: !addonsView.isInstalling
                checked: model.checked
                onClicked: addonsModel.changeState(packageName, checked)
            }
            QIconItem {
                icon: "applications-other"
                smooth: true
                Layout.minimumWidth: content.implicitHeight
                Layout.minimumHeight: content.implicitHeight
                opacity: addonsView.isInstalling ? 0.3 : 1
            }

            ColumnLayout {
                id: content
                Layout.fillWidth: true
                spacing: 0
                Label {
                    Layout.fillWidth: true
                    elide: Text.ElideRight
                    text: display
                }
                Label {
                    Layout.fillWidth: true
                    elide: Text.ElideRight
                    font.italic: true
                    text: toolTip
                }
            }
        }
    }
    
    RowLayout {
        visible: addonsModel.hasChanges && !addonsView.isInstalling
        spacing: 5

        Button {
            iconName: "dialog-ok"
            text: i18n("Apply Changes")
            onClicked: addonsModel.applyChanges()

            height: parent.visible ? implicitHeight : 0
        }
        Button {
            iconName: "document-revert"
            text: i18n("Discard")
            onClicked: addonsModel.discardChanges()

            height: parent.visible ? implicitHeight : 0
        }
    }
}
