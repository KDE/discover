import QtQuick 1.1
import org.kde.plasma.components 0.1
import org.kde.qtextracomponents 0.1

Item {
    id: window
    width: 800
    property Component applicationListComp: Qt.createComponent("qrc:/qml/ApplicationsListPage.qml")
    property Component applicationComp: Qt.createComponent("qrc:/qml/ApplicationPage.qml")
    property Component categoryComp: Qt.createComponent("qrc:/qml/CategoryPage.qml")
    
    //toplevels
    property Component welcomeComp: Qt.createComponent("qrc:/qml/WelcomePage.qml")
    property Component browsingComp: Qt.createComponent("qrc:/qml/BrowsingPage.qml")
    property Component installedComp: Qt.createComponent("qrc:/qml/InstalledPage.qml")
    property Component updatesComp: Qt.createComponent("qrc:/qml/UpdatesPage.qml")
    property Component currentTopLevel
    
    onCurrentTopLevelChanged: {
        if(currentTopLevel.status==Component.Error) {
            console.log("status error: "+currentTopLevel.errorString())
        }
        
        if(contentItem.content)
            contentItem.content.destroy()
        
        var obj
        try {
            obj = currentTopLevel.createObject(contentItem)
            console.log("created "+currentTopLevel)
        } catch (e) {
            console.log("error: "+e)
            console.log("comp error: "+currentTopLevel.errorString())
        }
        
        if(obj.init)
            obj.init()
        
        contentItem.content=obj
    }
    
    //sebas's hack :D
    Rectangle {
        anchors.fill: parent
        color: theme.backgroundColor
        opacity: .2
        
        Rectangle {
            anchors.fill: parent
            color: theme.textColor
        }
    }
    
    ToolBar {
        id:toolbar
        z: 10
        height: 50
        width: parent.width
        anchors.top: parent.top
        
        Row {
            spacing: 5
            anchors {
                top: parent.top
                bottom: parent.bottom
                left: parent.left
            }
            MuonToolButton {
                height: parent.height
                icon: "tools-wizard"
                text: i18n("Get software")
                checked: currentTopLevel==welcomeComp
                onClicked: currentTopLevel=welcomeComp
            }
            MuonToolButton {
                height: parent.height
                icon: "category-show-all"
                text: i18n("Browse")
                checked: currentTopLevel==browsingComp
                onClicked: currentTopLevel=browsingComp
            }
            MuonToolButton {
                height: parent.height
                icon: "applications-other"
                text: i18n("Installed")
                checked: currentTopLevel==installedComp
                onClicked: currentTopLevel=installedComp
            }
            MuonToolButton {
                height: parent.height
                icon: "system-software-update"
                text: i18n("Updates")
                checked: currentTopLevel==updatesComp
                onClicked: currentTopLevel=updatesComp
                enabled: app.appBackend!=null && app.appBackend.updatesCount>0
                overlayText: enabled ? app.appBackend.updatesCount : ""
            }
        }
        
        Row {
            spacing: 5
            anchors {
                top: parent.top
                bottom: parent.bottom
                right: parent.right
            }
            
            MuonToolButton {
                id: progressButton
                icon: "download"
                height: parent.height
                checkable: true
                checked: progressBox.visible
                onClicked: progressBox.visible=!progressBox.visible
                visible: progressBox.active

                ProgressView {
                    id: progressBox
                    visible: false
                    anchors.horizontalCenter: progressButton.horizontalCenter
                    anchors.top: progressButton.bottom
                }
            }
            
            MuonToolButton {
                id: usersButton
                icon: "system-users"
                visible: window.state=="loaded"
                height: parent.height
                checkable: true
                checked: connectionBox.visible
                onClicked: connectionBox.visible=!connectionBox.visible
                
                RatingAndReviewsConnection {
                    id: connectionBox
                    visible: false
                    anchors.horizontalCenter: usersButton.horizontalCenter
                    anchors.top: usersButton.bottom
                }
            }
            
            Repeater {
                model: ["software_properties", "quit"]
                
                delegate: MuonToolButton {
                    property QtObject action: app.getAction(modelData)
                    height: parent.height
                    
                    onClicked: action.trigger()
                    enabled: action.enabled
                    icon: action.icon
                }
            }
        }
    }
    
    onStateChanged: { 
        if(state=="loaded") {
            connectionBox.init()
            currentTopLevel = welcomeComp
        }
    }
    
    Item {
        id: contentItem
        property Item content
        anchors {
            bottom: parent.bottom
            top: toolbar.bottom
            left: parent.left
            right: parent.right
            topMargin: 3
        }
        
        onContentChanged: content.anchors.fill=contentItem
    }
}