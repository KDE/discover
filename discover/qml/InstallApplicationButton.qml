import QtQuick 2.1
import QtQuick.Controls 1.1
import QtQuick.Layouts 1.1
import org.kde.discover 1.0

ConditionalLoader
{
    id: root
    property alias application: listener.resource
    readonly property alias isActive: listener.isActive
    readonly property alias progress: listener.progress
    property Component additionalItem: null
    property bool canUpgrade: true
    property bool fill: false

    TransactionListener {
        id: listener
    }

    function click() {
        if (!isActive) {
            item.click();
        }
    }

    condition: listener.isActive
    componentTrue: RowLayout {
        Label {
            Layout.fillHeight: true
            Layout.fillWidth: true
            text: listener.statusText
            verticalAlignment: Text.AlignVCenter
        }

        Button {
            Layout.fillHeight: true
            iconName: "dialog-cancel"
            enabled: listener.isCancellable
            onClicked: listener.cancel()
        }
    }

    componentFalse: RowLayout {
        function click() { button.clicked(); }

        Loader {
            Layout.fillWidth: root.fill
            Component {
                id: updateButton
                Button {
                    text: i18n("Update")
                    onClicked: ResourcesModel.installApplication(application)
                }
            }
            sourceComponent: (root.canUpgrade && application.canUpgrade) ? updateButton : root.additionalItem
        }
        Button {
            id: button
            enabled: !ResourcesModel.isFetching
            text: !application.isInstalled ? i18n("Install") : i18n("Remove")
            Layout.fillWidth: root.fill

            onClicked: {
                if(application.isInstalled)
                    ResourcesModel.removeApplication(application);
                else
                    ResourcesModel.installApplication(application);
            }
        }
    }
}
