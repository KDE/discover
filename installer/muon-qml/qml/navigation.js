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
