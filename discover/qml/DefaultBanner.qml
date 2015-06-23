import QtQuick 2.1
import QtQuick.Controls 1.1
import org.kde.kquickcontrolsaddons 2.0

Item
{
    QIconItem {
        id: icon
        icon: "kde"
        y: 30
        width: 200
        height: 200
    }
    
    Label {
        text: i18n("Welcome to\nMuon Discover!")
        anchors {
            topMargin: 50
            rightMargin: 50
            right: parent.right
            verticalCenter: icon.verticalCenter
            left: icon.right
        }
        font.pixelSize: parent.height/5
        horizontalAlignment: Text.AlignHCenter
    }
}
