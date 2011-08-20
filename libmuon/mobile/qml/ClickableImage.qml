import QtQuick 1.0
import MuonMobile 1.0

Image {
    id: image
    fillMode: Image.PreserveAspectFit
    asynchronous: true
    opacity: 0.0
    visible: true

    signal buttonClicked

    MouseArea {
        anchors.fill: parent
        acceptedButtons: Qt.LeftButton
        Component.onCompleted: clicked.connect(parent.buttonClicked)
    }

    MouseCursor {
        anchors.fill: parent
        shape: Qt.PointingHandCursor
    }

    transitions: Transition {
                    from: "*"
                    to: "loaded"
                    PropertyAnimation { target: image; property: "opacity"; duration: 500 }
                }

    states: [
        State {
            name: 'loaded'
            when: image.status == Image.Ready
            PropertyChanges { target: image; opacity: 1.0 }
        }
    ]
}
