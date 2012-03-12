function clearOpened(immediate) {
    while(breadcrumbs.count>1) {
        pageStack.pop(null, breadcrumbs.count>2 || immediate)
        breadcrumbs.popItem()
    }
}

function openUpdatePage() {
    clearOpened(true)
    openPage("view-refresh", i18n("Updates..."), updatesComp, {})
}

function openInstalledList() {
    clearOpened(true)
    var obj = openPage("applications-other", i18n("Installed Applications"), applicationListComp, {})
    obj.stateFilter = (1<<8)
}

function openApplicationList(icon, name, cat, search) {
    var obj = openPage(icon, name, applicationListComp, { category: cat })
    if(search)
        obj.searchFor(search)
}

function openCategory(icon, name, cat) {
    openPage(icon, name, categoryComp, { category: cat })
}

function openApplication(app) {
    openPage(app.icon, app.name, applicationComp, { application: app })
}

function openPage(icon, name, component, props) {
    //due to animations, it can happen that the user clicks twice at the same button
    if(breadcrumbs.currentItem()==name || pageStack.busy)
        return
    
    var obj
    try {
        obj = component.createObject(pageStack, props)
        pageStack.push(obj);
        breadcrumbs.pushItem(icon, name)
        console.log("opened "+name)
    } catch (e) {
        console.log("error: "+e)
        console.log("comp error: "+component.errorString())
    }
    return obj
}
