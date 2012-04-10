import QtQuick 1.1
import org.kde.plasma.components 0.1
import "navigation.js" as Navigation

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
        
        try {
            var obj = currentTopLevel.createObject(pageStack)
            pageStack.initialPage = obj
//             console.log("created "+currentTopLevel)
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
                        Navigation.clearPages()
                        if(currentTopLevel!=modelData.component)
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
        onOpenApplicationInternal: Navigation.openApplication(app.appBackend.applicationByPackageName(appname))
    }
    
    ToolBar {
        id: breadcrumbsItemBar
        anchors {
            top: toolbar.bottom
            left: parent.left
            right: pageToolBar.left
            rightMargin: pageToolBar.visible ? 10 : 0
        }
        z: 0
        height: 30
        
        tools: Item {
            anchors.fill: parent
            Breadcrumbs {
                anchors {
                    left: parent.left
                    right: searchField.right
                    top: parent.top
                    bottom: parent.bottom
                }
                id: breadcrumbsItem
                pageStack: pageStack
                
                Component.onCompleted: breadcrumbsItem.pushItem("go-home")
            }
            
            TextField {
                id: searchField
                anchors {
                    right: parent.right
                    verticalCenter: parent.verticalCenter
                }
                visible: pageStack.currentPage!=null && pageStack.currentPage.searchFor!=null
                placeholderText: i18n("Search...")
                onTextChanged: if(text.length>3) pageStack.currentPage.searchFor(text)
//                 onAccepted: pageStack.currentPage.searchFor(text) //TODO: uncomment on kde 4.9
            }
        }
    }
    
    ToolBar {
        id: pageToolBar
        anchors {
            top: toolbar.bottom
            bottom: breadcrumbsItemBar.bottom
            right: parent.right
        }
        width: visible ? tools.childrenRect.width : 0
        visible: tools!=null
        
        Behavior on width { NumberAnimation { duration: 250 } }
    }
    
    PageStack {
        id: pageStack
        clip: true
        toolBar: pageToolBar
        anchors {
            bottom: progressBox.top
            top: breadcrumbsItemBar.bottom
            left: parent.left
            right: parent.right
            topMargin: 3
        }
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