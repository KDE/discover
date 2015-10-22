import QtQuick 2.1

Flickable {
    id: flick
    
    clip: true
    contentY: 0
    interactive: false
    contentWidth: Math.max(image.width, width)
    contentHeight: Math.max(image.height, height)
    property bool isCurrentItem: itemDelegate.PathView.isCurrentItem

    states: [
        State {
            name: "shownSmall"
            when: image.status==Image.Ready && image.height<(flick.height-titleBar.height)
            PropertyChanges { target: flick; contentY: (flick.contentHeight+titleBar.height)/2-flick.height/2 }
        },
        State {
            name: "shownIdeal"
            when: image.status==Image.Ready && image.height<flick.height
            PropertyChanges { target: flick; contentY: image.height-height }
        },
        State {
            name: "shownBig"
            when: flick.isCurrentItem
            PropertyChanges { target: flick; contentY: (flick.contentHeight-flick.height)/2 }
        },
        State {
            name: "notShown"
            when: !flick.isCurrentItem
            PropertyChanges { target: flick; contentY: 0 }
        }
    ]
    transitions: [
        Transition {
            from: "notShown"; to: "shownBig"
            NumberAnimation {
                properties: "contentY"
                duration: info.slideDuration
                easing.type: Easing.InOutQuad
            }
        }
    ]

    onIsCurrentItemChanged: {
        if (flick.isCurrentItem && image.status==Image.Error)
            info.next();
    }
    
    Image {
        id: image
        source: modelData.image
        x: -(image.width-flick.width)/2
    }
}
