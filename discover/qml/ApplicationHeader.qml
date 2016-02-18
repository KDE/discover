import QtQuick 2.1
import QtQuick.Controls 1.1
import QtQuick.Window 2.1
import QtQuick.Layouts 1.1
import org.kde.kquickcontrolsaddons 2.0

PageHeader {
    property alias application: installButton.application
    readonly property alias isInstalling: installButton.isActive

    RowLayout {
        anchors.fill: parent

        QIconItem {
            id: icon
            Layout.preferredHeight: parent.height
            Layout.preferredWidth: parent.height

            icon: application.icon
        }

        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.maximumHeight: parent.height
            spacing: 0
            Heading {
                id: heading
                text: application.name
                Layout.fillWidth: true
                elide: Text.ElideRight
                font.bold: true
            }
            Label {
                Layout.fillWidth: true
                text: application.comment
                wrapMode: Text.WordWrap
                elide: Text.ElideRight
                maximumLineCount: 1
//                         verticalAlignment: Text.AlignVCenter
            }
        }
        InstallApplicationButton {
            id: installButton
            application: appInfo.application
            additionalItem:  Rating {
                readonly property QtObject ratingInstance: application.rating
                visible: ratingInstance!=null
                rating:  ratingInstance==null ? 0 : ratingInstance.rating
            }
        }
    }
}
