import QtQuick 2.1
import QtQuick.Controls 1.1
import QtQuick.Layouts 1.1
import org.kde.muon 1.0

ConditionalLoader
{
    id: root
    property alias application: listener.resource
    property alias isActive: listener.isActive
    property Component additionalItem: null

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

    Rectangle { color: "red"; anchors.fill: parent }

    componentFalse: RowLayout {
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
        Loader {
            Component {
                id: updateButton
                Button {
                    text: i18n("Update")
                    onClicked: ResourcesModel.installApplication(application)
                }
            }
            sourceComponent: application.canUpgrade ? updateButton : root.additionalItem
        }
    }
}
