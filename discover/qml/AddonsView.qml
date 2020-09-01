import QtQuick 2.1
import QtQuick.Controls 2.14
import QtQuick.Layouts 1.1
import org.kde.discover 2.0
import "navigation.js" as Navigation
import org.kde.kirigami 2.14 as Kirigami

Kirigami.OverlaySheet
{
    id: addonsView
    parent: applicationWindow().overlay

    property alias application: addonsModel.application
    property bool isInstalling: false
    readonly property bool containsAddons: listview.count > 0 || isExtended
    readonly property bool isExtended: ResourcesModel.isExtended(application.appstreamId)

    header: Kirigami.Heading { text: i18n("Addons") }

    ListView
    {
        id: listview

        implicitWidth: Kirigami.Units.gridUnit * 25

        visible: addonsView.containsAddons
        enabled: !addonsView.isInstalling

        model: ApplicationAddonsModel { id: addonsModel }

        delegate: Kirigami.CheckableListItem {
            id: listItem

            enabled: !addonsView.isInstalling

            icon: undefined
            label: model.display
            subtitle: model.toolTip

            checked: model.checked

            action: Action {
                onTriggered: {
                    checked = !checked
                    addonsModel.changeState(packageName, listItem.checked)
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
