import QtQuick 1.1
import org.kde.plasma.components 0.1
import org.kde.qtextracomponents 0.1

Rectangle {
    width: 800
    height: 600
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
                    width: height; height: 30
                    
                    onClicked: modelData.trigger()
                    enabled: modelData.enabled
                    
                    QIconItem {
                        anchors.margins: 5
                        anchors.fill: parent
                        icon: modelData.icon
                    }
                }
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