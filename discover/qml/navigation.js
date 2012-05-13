function openApplicationList(icon, name, cat, search) {
    openPage(icon, name, applicationListComp, { category: cat, search: search })
}

function openApplicationListSource(uri) {
    openPage("view-filter", uri, applicationListComp, { originHostFilter: uri, preferList: true })
}

function openCategory(icon, name, cat) {
    openPage(icon, name, categoryComp, { category: cat })
}

function openApplication(app) {
    openPage(app.icon, app.name, applicationComp, { application: app })
}

function openPage(icon, name, component, props) {
    if(breadcrumbsItem.currentItem()==name || pageStack.busy)
        return
    
    var obj
    try {
        obj = component.createObject(pageStack.currentPage, props)
        pageStack.push(obj);
        breadcrumbsItem.pushItem(icon, name)
        console.log("opened "+name)
    } catch (e) {
        console.log("error: "+e)
        console.log("comp error: "+component.errorString())
    }
    return obj
}

function clearPages()
{
    breadcrumbsItem.doClick(0)
}