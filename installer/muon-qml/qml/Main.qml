import QtQuick 1.1
import org.kde.plasma.components 0.1
import org.kde.qtextracomponents 0.1

Rectangle {
    id: window
    width: 800
    height: 600
    color: "lightgrey"
    property Component categoryComp: Qt.createComponent("qrc:/qml/CategoryPage.qml")
    property Component applicationListComp: Qt.createComponent("qrc:/qml/ApplicationsListPage.qml")
    property Component applicationComp: Qt.createComponent("qrc:/qml/ApplicationPage.qml")
    property Component updatesComp: Qt.createComponent("qrc:/qml/UpdatesPage.qml")
    property bool opening: false
    
    function clearOpened() {
        while(breadcrumbs.count>1) {
            pageStack.pop(); breadcrumbs.popItem()
        }
    }
    
    function openUpdatePage() {
        clearOpened()
        openPage("view-refresh", i18n("Updates..."), updatesComp, {}, true)
    }
    
    function openInstalledList() {
        clearOpened()
        var obj = openPage("applications-other", i18n("Installed Applications"), applicationListComp, {}, true)
        obj.stateFilter = (1<<8)
    }
    
    function openApplicationList(icon, name, cat, search) {
        obj=openPage(icon, name, applicationListComp, { category: cat }, true)
        if(search)
            obj.searchFor(search)
    }
    
    function openCategory(icon, name, cat) {
        openPage(icon, name, categoryComp, { category: cat }, true)
    }
    
    function openApplication(app) {
        openPage(app.icon, app.name, applicationComp, { application: app }, false)
    }
    
    function openPage(icon, name, component, props, search) {
        //due to animations, it can happen that the user clicks twice at the same button
        if(breadcrumbs.currentItem()==name || opening)
            return
        opening=true
        
        var obj
        try {
            obj = component.createObject(pageStack, props)
            pageStack.push(obj);
            breadcrumbs.pushItem(icon, name, search)
            console.log("opened "+name)
        } catch (e) {
            console.log("error: "+e)
            console.log("comp error: "+component.errorString())
        } finally {
            opening=false
        }
        return obj
    }
    
    ToolBar {
        id:toolbar
        z: 10
        height: 40
        width: parent.width
        anchors.top: parent.top
        clip: true
        
        Row {
            id: tools
            spacing: 5
            anchors {
                top: parent.top
                bottom: parent.bottom
                left: parent.left
                topMargin: 5
            }
            
            Repeater {
                model: app.actions
                delegate: ToolButton {
                width: height; height: parent.height
                    
                    onClicked: modelData.trigger()
                    enabled: modelData.enabled
                    
                    QIconItem {
                        anchors.margins: 10
                        anchors.fill: parent
                        icon: modelData.icon
                    }
                }
            }
            
            ToolButton {
                width: height; height: parent.height
                iconSource: "applications-other"
                onClicked: openInstalledList()
            }
            
            ToolButton {
                width: height; height: parent.height
                iconSource: "view-refresh"
                onClicked: openUpdatePage()
            }
        }
        
        Breadcrumbs {
            id: breadcrumbs
            anchors.margins: 10
            anchors {
                top: parent.top
                bottom: parent.bottom
                right: parent.right
                left: tools.right
            }
            onClicked: {
                var pos = idx;
                while(pos--) { pageStack.pop(); breadcrumbs.popItem() }
            }
            
            onSearchChanged: {
                if(search)
                    pageStack.currentPage.searchFor(search)
            }
        }
    }
    
    onStateChanged: { 
        if(state=="loaded")
            breadcrumbs.pushItem("go-home", i18n("Get Software"), true)
    }
    
    CategoryPage {
        id: mainPage
    }
    
    PageStack
    {
        id: pageStack
        width: parent.width
        anchors.bottom: parent.bottom
        anchors.top: toolbar.bottom
        initialPage: window.state=="loaded" ? mainPage : null
        
        toolBar: toolbar
    }
}