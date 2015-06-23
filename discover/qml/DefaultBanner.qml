import QtQuick 2.1
import QtQuick.Controls 1.1
import org.kde.kquickcontrolsaddons 2.0

Item
{
    QIconItem {
        id: icon
        icon: "kde"
        anchors.verticalCenter: parent.verticalCenter
        width: parent.width/3
        height: width
    }
    
    Label {
        text: i18n("Welcome to\nMuon Discover!")
        anchors {
            right: parent.right
            top: parent.top
            bottom: parent.bottom
            left: icon.right
            rightMargin: 20
        }

        font.pixelSize: 72
        minimumPixelSize: 10
        verticalAlignment: Text.AlignVCenter

        horizontalAlignment: Text.AlignHCenter
        fontSizeMode: Text.Fit
    }
}
