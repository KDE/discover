import QtQuick 2.1
import QtQuick.Controls 2.1
import QtQuick.Layouts 1.1
import org.kde.discover 2.0
import "navigation.js" as Navigation
import org.kde.kirigami 2.0 as Kirigami

Kirigami.OverlaySheet
{
    id: addonsView
    property alias application: addonsModel.application
    property bool isInstalling: false
    readonly property bool containsAddons: rep.count>0 || isExtended
    readonly property bool isExtended: ResourcesModel.isExtended(application.appstreamId)

    header: Kirigami.Heading { text: i18n("Addons") }

    ColumnLayout
    {
        visible: addonsView.containsAddons
        enabled: !addonsView.isInstalling
        spacing: Kirigami.Units.largeSpacing

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
                        text: toolTip
                        opacity: 0.6
                    }
                }
            }
        }
    }

    footer: RowLayout {

        readonly property bool active: addonsModel.hasChanges && !addonsView.isInstalling

        Button {
            text: i18n("More...")
            visible: application.appstreamId.length>0 && addonsView.isExtended
            onClicked: Navigation.openExtends(application.appstreamId)
        }

        Item { Layout.fillWidth: true }

        Button {
            icon.name: "dialog-ok"
            text: i18n("Apply Changes")
            onClicked: addonsModel.applyChanges()

            enabled: parent.active
        }
        Button {
            icon.name: "document-revert"
            text: i18n("Reset")
            onClicked: addonsModel.discardChanges()

            enabled: parent.active
        }
    }
}
