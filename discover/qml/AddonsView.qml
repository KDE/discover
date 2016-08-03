import QtQuick 2.1
import QtQuick.Controls 1.1
import QtQuick.Layouts 1.1
import org.kde.discover 1.0
import org.kde.kquickcontrolsaddons 2.0
import "navigation.js" as Navigation
import org.kde.kirigami 1.0 as Kirigami

Kirigami.OverlaySheet
{
    id: addonsView
    property alias application: addonsModel.application
    property bool isInstalling: false
    readonly property bool isExtended: ResourcesModel.isExtended(application.appstreamId)

    ColumnLayout
    {
        visible: rep.count>0 || isExtended
        enabled: !addonsView.isInstalling
        spacing: 5

        Heading {
            text: i18n("Addons")
        }

        Repeater
        {
            id: rep
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
            readonly property bool active: addonsModel.hasChanges && !addonsView.isInstalling
            spacing: 5

            Button {
                iconName: "dialog-ok"
                text: i18n("Apply Changes")
                onClicked: addonsModel.applyChanges()

                visible: parent.active
            }
            Button {
                iconName: "document-revert"
                text: i18n("Discard")
                onClicked: addonsModel.discardChanges()

                visible: parent.active
            }
            Item {
                Layout.fillWidth: true
                height: 5
            }
            Button {
                text: i18n("More...")
                visible: application.appstreamId.length>0 && addonsView.isExtended
                onClicked: Navigation.openExtends(application.appstreamId)
            }
        }
    }
}
