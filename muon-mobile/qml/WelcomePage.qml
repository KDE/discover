import QtQuick 1.1
import org.kde.plasma.components 0.1
import org.kde.qtextracomponents 0.1
import "navigation.js" as Navigation

Item {
    function init() {
        breadcrumbsItem.pushItem("go-home")
    }
    
    Page {
        id: mainPage
        
        Information {
            id: info
            height: 200
            anchors {
                top: parent.top
                left: parent.left
                right: parent.right
            }
            
            dataModel: ListModel {
                ListElement { text: "KAlgebra"; color: "#770033"; icon: "kalgebra"; opacity: 0.2 }
                ListElement { text: "Digikam"; color: "#000088"; icon: "digikam"; opacity: 0.2 }
                ListElement { text: "Plasma"; color: "#003333"; icon: "plasma"; opacity: 0.2 }
            }
            
            delegate: Item {
                    property QtObject modelData
                    
                    Rectangle {
                        anchors.fill: parent
                        radius: 10
                        color: modelData.color
                        opacity: modelData.opacity
                    }
                    
                    Label {
                        anchors.margins: 20
                        anchors.right: parent.right
                        anchors.verticalCenter: parent.verticalCenter
                        horizontalAlignment: Text.AlignRight
                        verticalAlignment: Text.AlignVCenter
                        text: modelData.text
                        font.pointSize: info.height/3
                    }
                    
                    QIconItem {
                        anchors {
                            margins: 20
                            left: parent.left
                            top: parent.top
                            bottom: parent.bottom
                        }
                        width: height
                        icon: modelData.icon
                    }
            }
        }
        
        Component {
            id: topsDelegate
            Label { text: display }
        }
        
        ListView {
            id: top1
            clip: true
            anchors {
                margins: 5
                top: info.bottom
                left: parent.left
                bottom: parent.bottom
            }
            width: parent.width/4
            header: Label { text: i18n("<b>Most Downloads</b>") }
            model: ListModel {
                ListElement { display: "hola - 213123 downloads" }
                ListElement { display: "hola - 213123 downloads" }
                ListElement { display: "hola - 213123 downloads" }
                ListElement { display: "hola - 213123 downloads" }
                ListElement { display: "hola - 213123 downloads" }
                ListElement { display: "hola - 213123 downloads" }
                ListElement { display: "hola - 213123 downloads" }
            }
            delegate: topsDelegate
        }
        ListView {
            id: top2
            clip: true
            anchors {
                margins: 5
                top: info.bottom
                horizontalCenter: parent.horizontalCenter
                bottom: parent.bottom
            }
            width: parent.width/4
            header: Label { text: i18n("<b>Best Ratings</b>") }
            model: ListModel {
                ListElement { display: "hola - 5 stars" }
                ListElement { display: "hola - 5 stars" }
                ListElement { display: "hola - 5 stars" }
                ListElement { display: "hola - 5 stars" }
                ListElement { display: "hola - 5 stars" }
                ListElement { display: "hola - 5 stars" }
                ListElement { display: "hola - 5 stars" }
            }
            delegate: topsDelegate
        }
        ListView {
            id: top3
            clip: true
            anchors {
                margins: 5
                top: info.bottom
                right: parent.right
                bottom: parent.bottom
            }
            width: parent.width/4
            header: Label { text: i18n("<b>More Frequently Used</b>") }
            model: ListModel {
                ListElement { display: "hola - 1231231231 times" }
                ListElement { display: "hola - 1231231231 times" }
                ListElement { display: "hola - 1231231231 times" }
                ListElement { display: "hola - 1231231231 times" }
                ListElement { display: "hola - 1231231231 times" }
                ListElement { display: "hola - 1231231231 times" }
                ListElement { display: "hola - 1231231231 times" }
                ListElement { display: "hola - 1231231231 times" }
                ListElement { display: "hola - 1231231231 times" }
            }
            delegate: topsDelegate
        }
    }
    
    ToolBar {
        id: breadcrumbsItemBar
        anchors {
            top: parent.top
            left: parent.left
            right: pageToolBar.left
            rightMargin: 10
        }
        height: 30
        z: 0
        
        Breadcrumbs {
            id: breadcrumbsItem
            anchors.fill: parent
            onClicked: Navigation.jumpToIndex(pageStack, breadcrumbsItem, idx)
        }
    }
    
    ToolBar {
        id: pageToolBar
        anchors {
            leftMargin: 10
            rightMargin: 10
            top: parent.top
            right: parent.right
        }
        width: visible ? parent.width/4 : 0
        visible: tools!=null
        
        Behavior on width {
            NumberAnimation { duration: 250 }
        }
    }
    
    PageStack
    {
        id: pageStack
        anchors {
            margins: 10
            left: parent.left
            right: parent.right
            bottom: parent.bottom
            top: breadcrumbsItemBar.bottom
        }
        property alias breadcrumbs: breadcrumbsItem
        
        initialPage: window.state=="loaded" ? mainPage : null
        clip: true
        
        toolBar: pageToolBar
    }
}