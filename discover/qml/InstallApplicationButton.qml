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

    TransactionListener {
        id: listener
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
            onClicked: ResourcesModel.cancelTransaction(application)
        }
    }

    componentFalse: RowLayout {
        Loader {
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
            enabled: !ResourcesModel.isFetching
            text: !application.isInstalled ? i18n("Install") : i18n("Remove")

            onClicked: {
                if(application.isInstalled)
                    ResourcesModel.removeApplication(application);
                else
                    ResourcesModel.installApplication(application);
            }
        }
    }
}
