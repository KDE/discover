function openApplicationList(stack, icon, name, cat, search) {
    var obj = openPage(stack, icon, name, applicationListComp, { category: cat })
    if(search)
        obj.searchFor(search)
}

function openCategory(stack, icon, name, cat) {
    openPage(stack, icon, name, categoryComp, { category: cat })
}

function openApplication(stack, app) {
    openPage(stack, app.icon, app.name, applicationComp, { application: app })
}

function openPage(stack, icon, name, component, props) {
    if(stack.breadcrumbs.currentItem()==name || pageStack.busy)
        return
    
    var obj
    try {
        obj = component.createObject(pageStack, props)
        pageStack.push(obj);
        stack.breadcrumbs.pushItem(icon, name)
        console.log("opened "+name)
    } catch (e) {
        console.log("error: "+e)
        console.log("comp error: "+component.errorString())
    }
    return obj
}

function jumpToIndex(pageStack, bread, idx)
{
    var pos = idx;
    for(; pos>0; --pos) {
        pageStack.pop(undefined, true)
        bread.popItem()
    }
}