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
    property Component sourcesComp: Qt.createComponent("qrc:/qml/SourcesPage.qml")
    property Component currentTopLevel: defaultStartup ? browsingComp : loadingComponent
    property bool defaultStartup: true
    property bool navigationEnabled: true
    
    Component {
        id: loadingComponent
        Page {
            Label {
                text: i18n("Loading...")
                font.pointSize: 52
                anchors.centerIn: parent
            }
        }
    }
    
    onCurrentTopLevelChanged: {
        if(currentTopLevel==null)
            return
        searchField.text=""
        if(currentTopLevel.status==Component.Error) {
            console.log("status error: "+currentTopLevel.errorString())
        }
        while(pageStack.depth>1) {
            pageStack.pop().destroy(1000)
        }
        
        try {
            var obj = currentTopLevel.createObject(pageStack)
            pageStack.push(obj)
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
            
            MuonToolButton {
                height: toplevelsRow.height
                icon: "go-previous"
                enabled: window.navigationEnabled && breadcrumbsItem.count>1
                onClicked: {
                    breadcrumbsItem.popItem(false)
                    searchField.text = ""
                }
            }
            
            Item {
                //we add some extra space
                width: 20
            }
            
            property list<TopLevelPageData> sectionsModel: [
                TopLevelPageData { icon: "tools-wizard"; text: i18n("Discover"); component: browsingComp },
                TopLevelPageData {
                                icon: "applications-other"; text: i18n("Installed"); component: installedComp
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
                    enabled: window.navigationEnabled
                    onClicked: {
                        Navigation.clearPages()
                        if(currentTopLevel!=modelData.component)
                            currentTopLevel=modelData.component;
                    }
                }
            }
        }
        
        TextField {
            id: searchField
            anchors {
                right: parent.right
                rightMargin: 10
                verticalCenter: parent.verticalCenter
            }
            visible: pageStack.currentPage!=null && pageStack.currentPage.searchFor!=null
            placeholderText: i18n("Search...")
            onTextChanged: if(text.length>3) pageStack.currentPage.searchFor(text)
//             onAccepted: pageStack.currentPage.searchFor(text) //TODO: uncomment on kde 4.9
        }
    }
    
    Connections {
        target: app
        onAppBackendChanged: window.state = "loaded"
        onOpenApplicationInternal: Navigation.openApplication(app.appBackend.applicationByPackageName(appname))
        onListMimeInternal: Navigation.openApplicationMime(mime)
        onListCategoryInternal: Navigation.openApplicationList(c.icon, c.name, c, "")
    }
    
    ToolBar {
        id: breadcrumbsItemBar
        anchors {
            top: toolbar.bottom
            left: parent.left
            right: pageToolBar.left
            rightMargin: pageToolBar.visible ? 10 : 0
        }
        height: breadcrumbsItem.count<=1 ? 0 : 30
        
        tools: Breadcrumbs {
                id: breadcrumbsItem
                anchors.fill: parent
                pageStack: pageStack
                onPoppedPages: searchField.text=""
                Component.onCompleted: breadcrumbsItem.pushItem("go-home")
            }
        Behavior on height { NumberAnimation { duration: 250 } }
    }
    
    ToolBar {
        id: pageToolBar
        anchors {
            top: toolbar.bottom
            right: parent.right
        }
        height: 30
        width: tools!=null ? tools.childrenRect.width+5 : 0
        visible: width>0
        
        Behavior on width { NumberAnimation { duration: 250 } }
    }
    
    PageStack {
        id: pageStack
        clip: true
        toolBar: pageToolBar
        anchors {
            bottom: progressBox.top
            top: pageToolBar.bottom
            left: parent.left
            right: parent.right
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