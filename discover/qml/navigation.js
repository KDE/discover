/*
 *   Copyright (C) 2012 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library/Lesser General Public License
 *   version 2, or (at your option) any later version, as published by the
 *   Free Software Foundation
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details
 *
 *   You should have received a copy of the GNU Library/Lesser General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

function openApplicationList(icon, name, cat, search) {
    openPage(icon, name, applicationListComp, { category: cat, search: search, preferList: search!="" })
}

function openApplicationListSource(origin) {
    openPage("view-filter", origin, applicationListComp, { originFilter: origin, preferList: true })
}

function openApplicationMime(mime) {
    openPage("document-open-data", mime, applicationListComp, { mimeTypeFilter: mime })
}

function openCategory(cat) {
    if(cat.hasSubCategories)
        openPage(cat.icon, cat.name, categoryComp, { category: cat })
    else
        openApplicationList(cat.icon, cat.name, cat, "")
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