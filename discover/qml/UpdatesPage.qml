import QtQuick 2.0
import QtQuick.Layouts 1.2
import QtQuick.Controls 1.2
import org.kde.muon 1.0
import org.kde.kquickcontrolsaddons 2.0
import org.kde.kcoreaddons 1.0

ConditionalLoader
{
    id: page

    readonly property var icon: "system-software-update"
    readonly property string title: i18n("System Update")
    readonly property real proposedMargin: (width-app.actualWidth)/2

    ResourcesUpdatesModel {
        id: resourcesUpdatesModel

        onFinished: page.Stack.view.pop()
    }

    UpdateModel {
        id: updateModel
        backend: resourcesUpdatesModel
    }

    condition: updateModel.hasUpdates || resourcesUpdatesModel.isProgressing
    componentTrue: PresentUpdatesPage {
        proposedMargin: page.proposedMargin
    }

    componentFalse: Item {
        ColumnLayout {
            width: app.actualWidth
            anchors.centerIn: parent
            BusyIndicator {
                id: busy
                visible: false
                enabled: visible
                anchors.horizontalCenter: parent.horizontalCenter
                width: icon.width
                height: icon.height
            }
            QIconItem {
                anchors.horizontalCenter: parent.horizontalCenter

                id: icon
                width: 128
                height: 128
            }
            Label {
                id: title
                Layout.fillWidth: true
                horizontalAlignment: Text.AlignHCenter
                font.pointSize: description.font.pointSize*1.5
            }
            Label {
                id: description
                Layout.fillWidth: true
                horizontalAlignment: Text.AlignHCenter
            }
        }

        readonly property var secSinceUpdate: resourcesUpdatesModel.secsToLastUpdate

        state: ( ResourcesModel.isFetching                  ? "fetching"
               : secSinceUpdate < 0                         ? "unknown"
               : secSinceUpdate < 1000 * 60 * 60 * 24       ? "uptodate"
               : secSinceUpdate < 1000 * 60 * 60 * 24 * 7   ? "medium"
               :                                              "low"
               )

        states: [
            State {
                name: "fetching"
                PropertyChanges { target: icon; visible: false }
                PropertyChanges { target: busy; visible: true }
                PropertyChanges { target: title; text: i18nc("@info", "Loading...") }
                PropertyChanges { target: description; text: "" }
            },
            State {
                name: "uptodate"
                PropertyChanges { target: icon; icon: "security-high" }
                PropertyChanges { target: title; text: i18nc("@info", "The software on this computer is up to date.") }
                PropertyChanges { target: description; text: i18nc("@info", "Last checked %1 ago.", Format.formatDecimalDuration(secSinceUpdate*1000, 0)) }
            },
            State {
                name: "medium"
                PropertyChanges { target: icon; icon: "security-medium" }
                PropertyChanges { target: title; text: i18nc("@info", "No updates are available.") }
                PropertyChanges { target: description; text: i18nc("@info", "Last checked %1 ago.", Format.formatDecimalDuration(secSinceUpdate*1000, 0)) }
            },
            State {
                name: "low"
                PropertyChanges { target: icon; icon: "security-low" }
                PropertyChanges { target: title; text: i18nc("@info", "The last check for updates was over a week ago.") }
                PropertyChanges { target: description; text: i18nc("@info", "Last checked %1 ago.", Format.formatDecimalDuration(secSinceUpdate*1000, 0)) }
            },
            State {
                name: "unknown"
                PropertyChanges { target: icon; icon: "security-low" }
                PropertyChanges { target: title; text: i18nc("@info", "It is unknown when the last check for updates was.") }
                PropertyChanges { target: description; text: i18nc("@info", "Please click <em>Check for Updates</em> to check.") }
            }
        ]
    }
}
