import QtQuick 2.1
import QtQuick.Controls 1.1
import QtQuick.Layouts 1.1

ColumnLayout
{
    id: page
    readonly property real proposedMargin: (width-app.actualWidth)/2
    readonly property string title: i18n("Updating...")
    readonly property string icon: "system-software-update"

    function start() {
        resourcesUpdatesModel.prepare()
        resourcesUpdatesModel.updateAll()
    }

    onVisibleChanged: window.navigationEnabled=!visible
    Binding {
        target: progressBox
        property: "enabled"
        value: !visible
    }

    ProgressBar {
        id: progress
        width: app.actualWidth

        value: resourcesUpdatesModel.progress
        minimumValue: 0
        maximumValue: 100
        indeterminate: resourcesUpdatesModel.progress==-1
        
        Label {
            anchors.centerIn: parent
            text: resourcesUpdatesModel.remainingTime
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            visible: text!=""
        }
    }

    ScrollView {
        Layout.fillWidth: true
        Layout.fillHeight: true

        ListView {
            id: messageFlickable
            width: app.actualWidth
            property bool userScrolled: false
            clip: true
            model: resourcesUpdatesModel
            delegate: Label {
                text: display
                height: paintedHeight
                wrapMode: Text.Wrap
                width: messageFlickable.width
            }
            onContentHeightChanged: {
                if(!userScrolled && contentHeight>height && !moving) {
                    contentY = contentHeight - height + anchors.topMargin/2
                }
            }

            //if the user scrolls down, the viewport will be back to following the new progress
            onMovementEnded: userScrolled = !messageFlickable.atYEnd
        }
    }
}
