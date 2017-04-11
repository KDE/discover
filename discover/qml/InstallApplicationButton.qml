import QtQuick 2.1
import QtQuick.Controls 1.1
import QtQuick.Layouts 1.1
import org.kde.discover 1.0
import org.kde.kirigami 2.0 as Kirigami

ConditionalLoader
{
    id: root
    property alias application: listener.resource
    readonly property alias isActive: listener.isActive
    readonly property alias progress: listener.progress
    readonly property alias listener: listener
    property Component additionalItem: null

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
        LabelBackground {
            Layout.fillWidth: true
            text: listener.statusText
            progress: listener.progress/100
        }

        ToolButton {
            Layout.fillHeight: true
            iconName: "dialog-cancel"
            enabled: listener.isCancellable
            onClicked: listener.cancel()
        }
    }

    componentFalse: Button {
        id: button
        function click() { button.clicked(); }

        enabled: application.state != AbstractResource.Broken
        text: !application.isInstalled ? i18n("Install") : i18n("Remove")

        onClicked: {
            if(application.isInstalled)
                ResourcesModel.removeApplication(application);
            else
                ResourcesModel.installApplication(application);
        }
    }
}
