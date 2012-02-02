import QtQuick 1.1
import org.kde.plasma.components 0.1

Rectangle {
    height: 400
    width: 500
    color: "lightgrey"
    property Component categoryComp: Qt.createComponent("qrc:/qml/CategoryView.qml")
    property Component applicationListComp: Qt.createComponent("qrc:/qml/ApplicationsList.qml")
    property Component applicationComp: Qt.createComponent("qrc:/qml/ApplicationView.qml")
    property bool opening: false
    
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
        } catch (e) {
            console.log("error: "+e)
            console.log("comp error: "+applicationComp.errorString())
        } finally {
            opening=false
        }
        console.log("opened "+name)
        return obj
    }
    
    ToolBar {
        id:toolbar
        z: 10
        height: 40
        width: parent.width
        anchors.top: parent.top
        clip: true
        
        Breadcrumbs {
            id: breadcrumbs
            anchors.fill: parent
            onClicked: {
                var pos = idx;
                while(pos--) { pageStack.pop(); breadcrumbs.popItem() }
            }
            
            onSearchChanged: {
                if(search)
                    pageStack.currentPage.searchFor(search)
            }
            
            Component.onCompleted: pushItem("go-home", i18n("Get Software"), true)
        }
    }
    
    PageStack
    {
        id: pageStack
        width: parent.width
        anchors.bottom: parent.bottom
        anchors.top: toolbar.bottom
        initialPage: CategoryView {}
        
        toolBar: toolbar
    }
}