import QtQuick 1.1
import org.kde.plasma.components 0.1
import org.kde.qtextracomponents 0.1
import "navigation.js" as Navigation

Item {
    id: parentItem
    property int elemHeight: 65
    property alias count: view.count
    property alias header: view.header
    property alias section: view.section
    property bool preferUpgrade: false
    property alias model: view.model
    property real actualWidth: width

    ListView
    {
        id: view
        anchors {
            top: parent.top
            left: parent.left
            bottom: parent.bottom
            right: scroll.left
        }
        spacing: 3
        snapMode: ListView.SnapToItem
        
        delegate: ListItem {
                width: parentItem.actualWidth
                anchors.horizontalCenter: parent.horizontalCenter
                property real contHeight: elemHeight*0.7
                height: elemHeight
                QIconItem {
                    id: icon
                    icon: model["icon"]; width: contHeight; height: contHeight
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.left: parent.left
                }
                
                QIconItem {
                    anchors.right: icon.right
                    anchors.bottom: icon.bottom
                    visible: installed && !(view.model.stateFilter&(1<<8))
                    icon: "dialog-ok"
                    height: 16
                    width: 16
                }
                Label {
                    anchors.top: icon.top
                    anchors.left: icon.right
                    anchors.right: ratingsItem.left
                    anchors.leftMargin: 5
                    font.pointSize: commentLabel.font.pointSize*1.7
                    elide: Text.ElideRight
                    text: name
                }
                Label {
                    id: commentLabel
                    anchors.bottom: icon.bottom
                    anchors.left: icon.right
                    anchors.leftMargin: 5
                    text: "<em>"+comment+"</em>"
                    opacity: delegateArea.containsMouse ? 1 : 0.2
                }
                Rating {
                    id: ratingsItem
                    anchors {
                        right: parent.right
                        top: parent.top
                    }
                    height: contHeight*.5
                    rating: model.rating
                }
                
                MouseArea {
                    id: delegateArea
                    anchors.fill: parent
                    onClicked: Navigation.openApplication(application)
                    hoverEnabled: true
                    Row {
                        height: contHeight*0.5
                        spacing: 5
                        anchors {
                            bottom: parent.bottom
                            right: parent.right
                        }
                            
                        Button {
                            text: i18n("Upgrade")
                            id: upgradeButton
                            width: ratingsItem.width
                            visible: model.application.canUpgrade
                            onClicked: app.appBackend.installApplication(model.application)
                        }
                        
                        InstallApplicationButton {
                            id: installButton
                            width: ratingsItem.width
                            height: upgradeButton.height
    //                         property bool isVisible: delegateArea.containsMouse && !installButton.canHide
    //                         opacity: isVisible ? 1 : 0
                            application: model.application
                        }
                    }
                }
            }
    }
    
    ScrollBar {
        id: scroll
        orientation: Qt.Vertical
        flickableItem: view
        anchors {
                top: parent.top
                right: parent.right
                bottom: parent.bottom
        }
    }
}