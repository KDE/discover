import QtQuick 1.1

Flickable {
    id: flick
    
    contentY: 0
    interactive: false
    contentWidth: Math.max(image.width, width)
    contentHeight: Math.max(image.height, height)
    
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
            when: itemDelegate.PathView.isCurrentItem
            PropertyChanges { target: flick; contentY: flick.contentHeight-flick.height }
        },
        State {
            name: "notShown"
            when: !itemDelegate.PathView.isCurrentItem
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
        },
        Transition {
            to: "notShown"; from: "shownBig"
            NumberAnimation {
                properties: "contentY"
                duration: info.slideDuration
                easing.type: Easing.InOutQuad
            }
        }
    ]
    
    Image {
        id: image
        source: modelData.image
        x: -(image.width-flick.width)/2
    }
}
