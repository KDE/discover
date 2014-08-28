import QtQuick 2.1

MuonToolButton {
    id: button
    property alias delegate: menuItem.delegate
    property alias model: menuItem.model
    
    checkable: true
    checked: false
    
    ListView {
        id: menuItem
        width: 100
        height: button.height*count
        clip: true
        anchors.right: parent.right
        anchors.top: parent.bottom
        visible: button.checked
        Rectangle {
            anchors.fill: parent
            radius: 10
            opacity: 0.4
            z: -33
        }
    }
}
