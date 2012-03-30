import QtQuick 1.1
import org.kde.plasma.components 0.1

Item {
    id: window
    state: "loading" //changed onAppBackendChanged
    property Component applicationListComp: Qt.createComponent("qrc:/qml/ApplicationsListPage.qml")
    property Component applicationComp: Qt.createComponent("qrc:/qml/ApplicationPage.qml")
    property Component categoryComp: Qt.createComponent("qrc:/qml/CategoryPage.qml")
    
    //toplevels
    property Component browsingComp: Qt.createComponent("qrc:/qml/BrowsingPage.qml")
    property Component installedComp: Qt.createComponent("qrc:/qml/InstalledPage.qml")
    property Component updatesComp: Qt.createComponent("qrc:/qml/UpdatesPage.qml")
    property Component sourcesComp: Qt.createComponent("qrc:/qml/SourcesPage.qml")
    property Component currentTopLevel: browsingComp
    
    onCurrentTopLevelChanged: {
        if(currentTopLevel==null)
            return
        
        if(currentTopLevel.status==Component.Error) {
            console.log("status error: "+currentTopLevel.errorString())
        }
        
        if(contentItem.content)
            contentItem.content.destroy()
        
        var obj
        try {
            obj = currentTopLevel.createObject(contentItem)
//             console.log("created "+currentTopLevel)
            if(obj.init)
                obj.init()
            
            contentItem.content=obj
        } catch (e) {
            console.log("error: "+e)
            console.log("comp error: "+currentTopLevel.errorString())
        }
    }
    
    Item {
        id:toolbar
        z: 10
        height: 50
        width: parent.width
        anchors.top: parent.top
        
        Row {
            id: toplevelsRow
            spacing: 5
            anchors {
                top: parent.top
                bottom: parent.bottom
                left: parent.left
            }
            property list<TopLevelPageData> sectionsModel: [
                TopLevelPageData { icon: "tools-wizard"; text: i18n("Discover"); component: browsingComp },
                TopLevelPageData { icon: "applications-other"; text: i18n("Installed"); component: installedComp },
                TopLevelPageData {
                                icon: "system-software-update"; text: i18n("Updates"); component: updatesComp
                                overlay: !app.appBackend || app.appBackend.updatesCount==0 ? "" : app.appBackend.updatesCount
                },
                TopLevelPageData { icon: "document-import"; text: i18n("Sources"); component: sourcesComp }
            ]
            Repeater {
                model: toplevelsRow.sectionsModel
                
                delegate: MuonToolButton {
                    height: toplevelsRow.height
                    text: modelData.text
                    icon: modelData.icon
                    overlayText: modelData.overlay
                    checked: currentTopLevel==modelData.component
                    onClicked: {
                        if(currentTopLevel=modelData.component)
                            currentTopLevel=null
                        currentTopLevel=modelData.component;
                    }
                }
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
                    anchors.right: usersButton.right
                    anchors.top: usersButton.bottom
                }
            }
        }
    }
    
    Connections {
        target: app
        onAppBackendChanged: {
            connectionBox.init()
            window.state = "loaded"
        }
        onOpenApplicationInternal: contentItem.content.openApplication(app)
    }
    
    Item {
        id: contentItem
        property Item content
        anchors {
            bottom: progressBox.top
            top: toolbar.bottom
            left: parent.left
            right: parent.right
            topMargin: 3
        }
        
        onContentChanged: content.anchors.fill=contentItem
    }
    
    ProgressView {
        id: progressBox
        anchors {
            left: parent.left
            right: parent.right
            bottom: parent.bottom
        }
    }
}