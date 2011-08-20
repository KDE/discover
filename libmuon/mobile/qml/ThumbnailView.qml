import QtQuick 1.0
import Effects 1.0

Rectangle {
    id: rectangle1
    width: 170
    height: 130

    property alias source: image.source
    signal thumbnailClicked()
    signal thumbnailLoaded()

    ClickableImage {
        id: image
        width: 160
        height: 120

        effect: DropShadow {
             blurRadius: 10
        }

        onStatusChanged: {
            if (image.status == Image.Error)
                view.hide()
            else if (image.status == Image.Ready)
                parent.thumbnailLoaded()
        }
        onButtonClicked: parent.thumbnailClicked()
    }
}
